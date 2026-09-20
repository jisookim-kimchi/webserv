#include "../includes/Server.hpp"
#include "../includes/CgiHandler.hpp"
#include "../includes/HTTP_Request.hpp"
#include "../includes/HttpResponse.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr int MAX_EVENTS = 256;
constexpr size_t BUF_SIZE = 4096;

Server::~Server() {}

/*
    @brief assign operator disabled to prevent duplicating server instance.
*/
const Server &Server::operator=(const Server &other) {
  (void)other;
  return *this;
}

/*
    @brief Copy constructor disabled to prevent duplicating server instance.
*/
Server::Server(const Server &other) { (void)other; }

/*
    @brief make socket for each ServerConfig and assign to listenSockets_
   vector.
    @param serverConfigs Vector of ServerConfig objects.
*/
Server::Server(const std::vector<ServerConfig> &serverConfigs)
    : serverConfigs_(serverConfigs) {
  for (const auto &serverConfig : serverConfigs_) {
    const std::string &host = serverConfig.getHost();
    const std::vector<uint16_t> &ports = serverConfig.getPort();
    for (const auto &port : ports) {
      std::unique_ptr<ListenSocket> socket = std::make_unique<ListenSocket>();
      socket->createSocket();
      socket->setSocketOption();
      socket->setNonBlocking();
      socket->bind(port, host);
      socket->listen();
      listenSockets_.push_back(std::move(socket));
    }
  }
}

/**
    @brief : check if the file descriptor is listen socket.
    @param fd : file descriptor
    @return : true if the file descriptor is listen socket, false otherwise
*/
bool Server::isListenSocket(int fd) {
  for (const auto &listenSocket : listenSockets_) {
    if (listenSocket->getFd() == fd) {
      return true;
    }
  }
  return false;
}

/**
    @brief : run Server event loop
            register listen socket to epoll event.
            wait for events and handle them.
    @param epollFd : epoll file descriptor
    @param events : epoll event array
    @param eventCount : number of events
*/
void Server::run() {
  int epollFd = epoll_create(1);
  if (epollFd == -1) {
    throw std::runtime_error("epoll create failed");
  }

  for (const auto &listenSocket : listenSockets_) {
    int fd = listenSocket->getFd();
    struct epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
      close(epollFd);
      throw std::runtime_error("epoll ctl add failed");
    }
  }

  struct epoll_event events[MAX_EVENTS]{};
  while (true) {
    int eventCount = epoll_wait(epollFd, events, MAX_EVENTS, -1);
    if (eventCount == -1) {
      if (errno == EINTR) {
        continue;
      }
      close(epollFd);
      throw std::runtime_error("epoll wait failed");
    }

    for (int i = 0; i < eventCount; i++) {
      int eventFd = events[i].data.fd;

      if (isListenSocket(eventFd)) {
        handleNewConnection(eventFd, epollFd);
      } else {
        auto it = clients_.find(eventFd);
        if (it == clients_.end())
          continue;

        if (events[i].events & EPOLLIN) {
          handleClientRead(eventFd, epollFd, it);
        } else if (events[i].events & EPOLLOUT) {
          handleClientWrite(eventFd, epollFd, it);
        }
      }
    }
  }
}

/**
    @brief : accept new client connection set non-blocking socket and register it to epoll.
                client managed by clients_map(key : client file descriptor, value : Client object)
    @param listenFd : listen socket file descriptor
    @param epollFd : epoll file descriptor
*/
void Server::handleNewConnection(int listenFd, int epollFd) {
  struct sockaddr_in clientAddr{};
  socklen_t clientAddrLen = sizeof(clientAddr);

  int clientFd =
      accept(listenFd, reinterpret_cast<struct sockaddr *>(&clientAddr),
             &clientAddrLen);
  if (clientFd == -1)
    return;

  int flags = fcntl(clientFd, F_GETFL, 0);
  if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
    close(clientFd);
    return;
  }

  int port = 0;
  for (const auto &ls : listenSockets_) {
    if (ls->getFd() == listenFd) {
      port = ls->getPort();
      break;
    }
  }

  clients_[clientFd] = std::make_unique<Client>(clientFd, clientAddr, port);
  struct epoll_event clientEvent{};
  clientEvent.events = EPOLLIN;
  clientEvent.data.fd = clientFd;
  epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent);
}

/**
    @brief :  Read HTTP request data from client socket and append to buffer.
                when the request header is reached to ("\r\n\r\n") it will call processRequest()
    @param clientFd : client file descriptor
    @param epollFd : epoll file descriptor
    @param it : iterator to the client in clients_map
*/
void Server::handleClientRead(
    int clientFd, int epollFd,
    std::map<int, std::unique_ptr<Client>>::iterator it) {
  Client &client = *(it->second);
  char buf[BUF_SIZE];
  ssize_t ret = read(clientFd, buf, BUF_SIZE);

  if (ret <= 0) {
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
    clients_.erase(it);
    return;
  }

  client.appendRequestBuffer(buf, ret);
  if (client.getRequestBuffer().find("\r\n\r\n") != std::string::npos) {
    processRequest(client, epollFd);
  }
}

/**
    @brief :  Parse HTTP request buffer and decide whether it is static file request or cgi request.
                If it's static file request, call handleStaticFile() else call handleCgi().
                if it's invalid, generate 400 error response and send it to client.
    @param client : client object
    @param epollFd : epoll file descriptor
    @note : now it hanlde only serverConfigs_[0] so we need to fix this part to handle multiple serverConfigs.
*/
void Server::processRequest(Client &client, int epollFd) {
  HTTP_Request req;
  if (req.parse(client.getRequestBuffer())) {
    HttpResponse::RequestView view;
    view.method = req.getMethodString();
    view.path = req.getPath();
    view.body = req.getBody();
    view.errorStatus = 0;

    HttpResponse res;
    res.buildForPath(view, serverConfigs_[0]);

    if (res.needsCgi()) {
      const LocationConfig *loc =
          LocationMatch::match(req.getPath(), serverConfigs_[0]);
      handleCgi(client, req, loc);
    } else {
      client.setResponseBuffer(res.getRaw());
    }
  } else {
    HttpResponse::RequestView view;
    view.errorStatus = 400;
    HttpResponse res;
    res.buildForPath(view, serverConfigs_[0]);
    client.setResponseBuffer(res.getRaw());
  }

  client.setState(ClientState::WRITING_RESPONSE);
  struct epoll_event ev{};
  ev.events = EPOLLOUT;
  ev.data.fd = client.getFd();
  epoll_ctl(epollFd, EPOLL_CTL_MOD, client.getFd(), &ev);
}

/**
    @brief : if it'scgi request, call this function.
                create a CgiHandler instance and request's parsing data insert into CgiHandler instance.
                call CgiHandler::execute() method and get the result.
                set client's response buffer with the result.
    @param client : client object
    @param req : HTTP request
    @param loc : LocationConfig object
*/
void Server::handleCgi(Client &client, const HTTP_Request &req,
                       const LocationConfig *loc) {
  CgiHandler::Request cgiReq;
  if (loc) {
    cgiReq.interpreterPath = loc->getCgiPass();
    std::string root = (loc && !loc->getRoot().empty()) ? loc->getRoot() : "www";
    cgiReq.documentRoot = root;
    // solved!:  pass real path cgiReq.scriptPath because when it call below cgiHandler::execute(), /home/jisookim/42webserv/www/cgi-bin/www/cgi-bin/hello.py.
    std::string fullPath = root;
    if (!req.getPath().empty() && req.getPath()[0] != '/')
      fullPath += "/";
    fullPath += req.getPath();
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != nullptr)
      cgiReq.scriptPath = std::string(cwd) + "/" + fullPath;
    else
      cgiReq.scriptPath = fullPath;
  }
  cgiReq.requestMethod = req.getMethodString();
  cgiReq.requestUri = req.getPath();
  cgiReq.queryString = req.getQueryString();
  cgiReq.contentType = req.getHeader("content-type");
  cgiReq.requestBody = req.getBody();
  cgiReq.headers = req.getHeaders();
  cgiReq.serverPort = std::to_string(client.getServerPort());
  cgiReq.serverName = serverConfigs_[0].getServerName().empty()
                          ? "localhost"
                          : serverConfigs_[0].getServerName()[0];

  CgiHandler::Result cgiResult = CgiHandler::execute(cgiReq); // check here!
  //TODO add HTTP format here!
  

  client.setResponseBuffer(cgiResult.rawOutput);
}

/**
    @brief : send response buffer to client and close the connection.
    @param clientFd : client file descriptor
    @param epollFd : epoll file descriptor
    @param it : iterator to the client in clients_map
*/
void Server::handleClientWrite(
    int clientFd, int epollFd,
    std::map<int, std::unique_ptr<Client>>::iterator it) {
  Client &client = *(it->second);
  const std::string &response = client.getResponseBuffer();
  std::cout << "HOOK: final packet :\n" << response << std::endl;
  send(clientFd, response.c_str(), response.length(), 0);

  epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
  clients_.erase(it);
}
