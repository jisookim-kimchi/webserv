#include "../includes/Server.hpp"

#include "../includes/HTTP_Request.hpp"
#include "../includes/HttpResponse.hpp"
#include "../includes/Utils.hpp"

#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr int MAX_EVENTS = 256;
constexpr size_t BUF_SIZE = 65536;
constexpr int CGI_POLL_TIMEOUT_MS = 200;

namespace {

std::string headerValue(const std::string& headersBlock, const std::string& keyLower) {
    size_t pos = 0;
    while (pos < headersBlock.size()) {
        size_t lineEnd = headersBlock.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            lineEnd = headersBlock.size();
        size_t colon = headersBlock.find(':', pos);
        if (colon != std::string::npos && colon < lineEnd) {
            std::string name = headersBlock.substr(pos, colon - pos);
            for (size_t i = 0; i < name.size(); ++i) {
                if (name[i] >= 'A' && name[i] <= 'Z')
                    name[i] |= 0x20;
            }
            if (name == keyLower) {
                size_t valStart = colon + 1;
                while (valStart < lineEnd &&
                       (headersBlock[valStart] == ' ' || headersBlock[valStart] == '\t'))
                    ++valStart;
                return headersBlock.substr(valStart, lineEnd - valStart);
            }
        }
        if (lineEnd >= headersBlock.size())
            break;
        pos = lineEnd + 2;
    }
    return "";
}

bool parseSizeDecimal(const std::string& s, size_t& out) {
    if (s.empty())
        return false;
    out = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
        out = out * 10 + static_cast<size_t>(s[i] - '0');
    }
    return true;
}

bool parseHexSize(const std::string& s, size_t& out) {
    if (s.empty())
        return false;
    out = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        size_t digit = 0;
        if (c >= '0' && c <= '9')
            digit = static_cast<size_t>(c - '0');
        else {
            char lower = static_cast<char>(c | 0x20);
            if (lower >= 'a' && lower <= 'f')
                digit = static_cast<size_t>(lower - 'a' + 10);
            else
                return false;
        }
        out = (out << 4) | digit;
    }
    return true;
}

bool isRequestComplete(const std::string& buf) {
    const size_t headerEnd = buf.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    const std::string headers = buf.substr(0, headerEnd);
    const size_t bodyStart = headerEnd + 4;
    const std::string te = headerValue(headers, "transfer-encoding");
    const std::string cl = headerValue(headers, "content-length");
    const bool chunked = (te == "chunked");

    if (!cl.empty() && chunked)
        return true;

    if (chunked) {
        size_t pos = bodyStart;
        while (pos < buf.size()) {
            size_t rn = buf.find("\r\n", pos);
            if (rn == std::string::npos)
                return false;
            size_t chunkSize = 0;
            if (!parseHexSize(buf.substr(pos, rn - pos), chunkSize))
                return true;
            if (chunkSize == 0)
                return rn + 4 <= buf.size();
            size_t dataStart = rn + 2;
            if (dataStart + chunkSize + 2 > buf.size())
                return false;
            pos = dataStart + chunkSize + 2;
        }
        return false;
    }

    if (!cl.empty()) {
        size_t expected = 0;
        if (!parseSizeDecimal(cl, expected))
            return true;
        return buf.size() >= bodyStart + expected;
    }

    return true;
}

std::string resolveScriptPath(const HttpResponse& mapper, const ServerConfig& server,
                              const LocationConfig& loc, const std::string& urlPath) {
    std::string fs = mapper.mapUrlToFs(loc, urlPath);
    if (!fs.empty())
        return fs;

    const LocationConfig* best = nullptr;
    size_t bestLen = 0;
    for (const LocationConfig& candidate : server.getLocations()) {
        const std::string& lp = candidate.getPath();
        if (lp.empty() || lp[0] == '.' || candidate.getRoot().empty())
            continue;
        if (urlPath.compare(0, lp.size(), lp) != 0)
            continue;
        if (lp != "/" && urlPath.size() > lp.size() && urlPath[lp.size()] != '/')
            continue;
        if (lp.size() >= bestLen) {
            bestLen = lp.size();
            best = &candidate;
        }
    }
    if (best == nullptr)
        return "";
    return mapper.mapUrlToFs(*best, urlPath);
}

uint32_t pollToEpoll(short pevents) {
    uint32_t events = 0;
    if (pevents & POLLIN)
        events |= EPOLLIN;
    if (pevents & POLLOUT)
        events |= EPOLLOUT;
    return events;
}

std::string formatCgiHttp(const CgiHandler::Result& cgiResult) {
    std::string out = "HTTP/1.1 " + std::to_string(cgiResult.statusCode) + " " +
                      HttpResponse::statusText(cgiResult.statusCode) + "\r\n";
    bool hasCL = false;
    for (const auto& h : cgiResult.headers) {
        if (Utils::toLower(h.first) == "status")
            continue;
        out += h.first + ": " + h.second + "\r\n";
        if (Utils::toLower(h.first) == "content-length")
            hasCL = true;
    }
    if (!hasCL)
        out += "Content-Length: " + std::to_string(cgiResult.body.size()) + "\r\n";
    out += "Connection: close\r\n\r\n";
    out += cgiResult.body;
    return out;
}

}  // namespace

Server::~Server() {}

const Server& Server::operator=(const Server& other) {
    (void)other;
    return *this;
}

Server::Server(const Server& other) {
    (void)other;
}

Server::Server(const std::vector<ServerConfig>& serverConfigs) : serverConfigs_(serverConfigs) {
    for (const auto& serverConfig : serverConfigs_) {
        const std::string& host = serverConfig.getHost();
        const std::vector<uint16_t>& ports = serverConfig.getPort();
        for (const auto& port : ports) {
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

bool Server::isListenSocket(int fd) {
    for (const auto& listenSocket : listenSockets_) {
        if (listenSocket->getFd() == fd)
            return true;
    }
    return false;
}

bool Server::hasActiveCgi() const {
    for (const auto& entry : clients_) {
        if (entry.second->getState() == ClientState::CGI_RUNNING && entry.second->cgi() != nullptr)
            return true;
    }
    return false;
}

void Server::run() {
    int epollFd = epoll_create(1);
    if (epollFd == -1)
        throw std::runtime_error("epoll create failed");

    for (const auto& listenSocket : listenSockets_) {
        int fd = listenSocket->getFd();
        struct epoll_event event {};
        event.events = EPOLLIN;
        event.data.fd = fd;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
            close(epollFd);
            throw std::runtime_error("epoll ctl add failed");
        }
    }

    struct epoll_event events[MAX_EVENTS]{};
    while (true) {
        const int timeout = hasActiveCgi() ? CGI_POLL_TIMEOUT_MS : -1;
        int eventCount = epoll_wait(epollFd, events, MAX_EVENTS, timeout);
        if (eventCount == -1) {
            if (errno == EINTR)
                continue;
            close(epollFd);
            throw std::runtime_error("epoll wait failed");
        }

        for (int i = 0; i < eventCount; i++) {
            int eventFd = events[i].data.fd;

            if (isListenSocket(eventFd)) {
                handleNewConnection(eventFd, epollFd);
                continue;
            }

            auto cgiIt = cgiPipeToClient_.find(eventFd);
            if (cgiIt != cgiPipeToClient_.end()) {
                handleCgiPipeEvent(eventFd, events[i].events, epollFd);
                continue;
            }

            auto it = clients_.find(eventFd);
            if (it == clients_.end())
                continue;

            if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                closeClient(it, epollFd);
                continue;
            }
            if (events[i].events & EPOLLIN)
                handleClientRead(eventFd, epollFd, it);
            else if (events[i].events & EPOLLOUT)
                handleClientWrite(eventFd, epollFd, it);
        }

        checkCgiTimeouts(epollFd);
    }
}

void Server::handleNewConnection(int listenFd, int epollFd) {
    struct sockaddr_in clientAddr {};
    socklen_t clientAddrLen = sizeof(clientAddr);

    int clientFd =
        accept(listenFd, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
    if (clientFd == -1)
        return;

    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(clientFd);
        return;
    }
    int opt = 1;
    setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    int port = 0;
    for (const auto& ls : listenSockets_) {
        if (ls->getFd() == listenFd) {
            port = ls->getPort();
            break;
        }
    }

    clients_[clientFd] = std::make_unique<Client>(clientFd, clientAddr, port);
    struct epoll_event clientEvent {};
    clientEvent.events = EPOLLIN;
    clientEvent.data.fd = clientFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent);
}

void Server::closeClient(std::map<int, std::unique_ptr<Client>>::iterator it, int epollFd) {
    Client& client = *(it->second);
    // Drop any CGI pipe watches for this client.
    for (auto pit = cgiPipeToClient_.begin(); pit != cgiPipeToClient_.end();) {
        if (pit->second == client.getFd()) {
            epoll_ctl(epollFd, EPOLL_CTL_DEL, pit->first, nullptr);
            pit = cgiPipeToClient_.erase(pit);
        } else {
            ++pit;
        }
    }
    epoll_ctl(epollFd, EPOLL_CTL_DEL, client.getFd(), nullptr);
    clients_.erase(it);
}

void Server::armClientWrite(Client& client, int epollFd) {
    client.setState(ClientState::WRITING_RESPONSE);
    struct epoll_event ev {};
    ev.events = EPOLLOUT;
    ev.data.fd = client.getFd();
    epoll_ctl(epollFd, EPOLL_CTL_MOD, client.getFd(), &ev);
}

void Server::handleClientRead(int clientFd, int epollFd,
                              std::map<int, std::unique_ptr<Client>>::iterator it) {
    Client& client = *(it->second);
    if (client.getState() == ClientState::WRITING_RESPONSE)
        return;

    char buf[BUF_SIZE];
    ssize_t ret = read(clientFd, buf, BUF_SIZE);
    if (ret <= 0) {
        closeClient(it, epollFd);
        return;
    }

    // Ignore extra client input while CGI is in flight.
    if (client.getState() == ClientState::CGI_RUNNING)
        return;

    client.appendRequestBuffer(buf, static_cast<size_t>(ret));
    if (!isRequestComplete(client.getRequestBuffer()))
        return;
    processRequest(client, epollFd);
}

void Server::processRequest(Client& client, int epollFd) {
    HTTP_Request req;
    if (req.parse(client.getRequestBuffer())) {
        const ServerConfig& config = findServerConfig(client, req);
        HttpResponse::RequestView view;
        view.method = req.getMethodString();
        view.path = req.getPath();
        view.body = req.getBody();
        view.errorStatus = 0;

        HttpResponse res;
        res.buildForPath(view, config);

        if (res.needsCgi()) {
            const LocationConfig* loc = LocationMatch::match(req.getPath(), config);
            handleCgi(client, req, loc, epollFd);
            return;
        }
        client.setResponseBuffer(res.getRaw());
    } else {
        HttpResponse::RequestView view;
        view.errorStatus = 400;
        HttpResponse res;
        res.buildForPath(view, serverConfigs_[0]);
        client.setResponseBuffer(res.getRaw());
    }

    armClientWrite(client, epollFd);
}

const ServerConfig& Server::findServerConfig(const Client& client, const HTTP_Request& req) const {
    int clientPort = client.getServerPort();
    std::string hostHeader = req.getHeader("host");
    size_t colon = hostHeader.find(':');
    size_t hostNameLen = (colon != std::string::npos) ? colon : hostHeader.length();
    const ServerConfig* defaultServer = nullptr;
    for (const auto& config : serverConfigs_) {
        const auto& ports = config.getPort();
        bool portMatch = false;
        for (uint16_t p : ports) {
            if (p == clientPort) {
                portMatch = true;
                break;
            }
        }
        if (!portMatch)
            continue;
        if (!defaultServer)
            defaultServer = &config;
        for (const auto& name : config.getServerName()) {
            if (name.length() == hostNameLen && hostHeader.compare(0, hostNameLen, name) == 0)
                return config;
        }
    }
    return defaultServer ? *defaultServer : serverConfigs_[0];
}

CgiHandler::Request Server::createCgiRequest(Client& client, const HTTP_Request& req,
                                             const LocationConfig* loc) const {
    CgiHandler::Request cgiReq;
    const ServerConfig& config = findServerConfig(client, req);

    if (loc != nullptr) {
        cgiReq.interpreterPath = loc->getCgiPass();
        HttpResponse mapper;
        const std::string scriptPath = resolveScriptPath(mapper, config, *loc, req.getPath());
        cgiReq.scriptPath = scriptPath;
        cgiReq.documentRoot =
            loc->getRoot().empty() ? Utils::dirName(scriptPath) : loc->getRoot();
        cgiReq.workingDirectory = Utils::dirName(scriptPath);
    }

    cgiReq.requestMethod = req.getMethodString();
    cgiReq.requestUri = req.getPath();
    if (!req.getQueryString().empty())
        cgiReq.requestUri += "?" + req.getQueryString();
    cgiReq.queryString = req.getQueryString();
    cgiReq.contentType = req.getHeader("content-type");
    cgiReq.requestBody = req.getBody();
    cgiReq.headers = req.getHeaders();
    cgiReq.scriptName = req.getPath();
    cgiReq.serverPort = std::to_string(client.getServerPort());
    cgiReq.serverName =
        config.getServerName().empty() ? "localhost" : config.getServerName()[0];
    return cgiReq;
}

// Clear this client's pipe fds from epoll, then re-add whatever CgiHandler still wants.
void Server::syncCgiEpoll(Client& client, int epollFd) {
    const int clientFd = client.getFd();
    for (auto it = cgiPipeToClient_.begin(); it != cgiPipeToClient_.end();) {
        if (it->second == clientFd) {
            epoll_ctl(epollFd, EPOLL_CTL_DEL, it->first, nullptr);
            it = cgiPipeToClient_.erase(it);
        } else {
            ++it;
        }
    }
    if (client.cgi() == nullptr)
        return;

    CgiHandler& cgi = *client.cgi();
    auto addPipe = [&](int fd, short want) {
        if (fd < 0 || want == 0)
            return;
        struct epoll_event ev {};
        ev.events = pollToEpoll(want);
        ev.data.fd = fd;
        cgiPipeToClient_[fd] = clientFd;
        epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
    };
    addPipe(cgi.stdinFd(), cgi.stdinEvents());
    addPipe(cgi.stdoutFd(), cgi.stdoutEvents());
}

void Server::finishCgi(Client& client, int epollFd) {
    // Remove pipe watches first (fds may already be closed by CgiHandler).
    for (auto it = cgiPipeToClient_.begin(); it != cgiPipeToClient_.end();) {
        if (it->second == client.getFd()) {
            epoll_ctl(epollFd, EPOLL_CTL_DEL, it->first, nullptr);
            it = cgiPipeToClient_.erase(it);
        } else {
            ++it;
        }
    }

    int err = 500;
    if (client.cgi() != nullptr) {
        const CgiHandler::State st = client.cgi()->state();
        if (st == CgiHandler::State::Done) {
            client.setResponseBuffer(formatCgiHttp(client.cgi()->result()));
            err = 0;
        } else if (st == CgiHandler::State::TimedOut) {
            err = 504;
        }
    }
    if (err != 0) {
        HttpResponse::RequestView view;
        view.errorStatus = err;
        HttpResponse res;
        res.buildForPath(view, serverConfigs_[0]);
        client.setResponseBuffer(res.getRaw());
    }
    client.clearCgi();
    armClientWrite(client, epollFd);
}

void Server::handleCgi(Client& client, const HTTP_Request& req, const LocationConfig* loc,
                       int epollFd) {
    const ServerConfig& config = findServerConfig(client, req);
    try {
        client.setCgi(std::make_unique<CgiHandler>());
        client.cgi()->launch(createCgiRequest(client, req, loc));
        client.setState(ClientState::CGI_RUNNING);
        syncCgiEpoll(client, epollFd);

        struct epoll_event ev {};
        ev.events = EPOLLIN | EPOLLRDHUP;
        ev.data.fd = client.getFd();
        epoll_ctl(epollFd, EPOLL_CTL_MOD, client.getFd(), &ev);

        if (client.cgi()->isFinished())
            finishCgi(client, epollFd);
    } catch (const std::exception&) {
        client.clearCgi();
        HttpResponse::RequestView view;
        view.errorStatus = 500;
        HttpResponse res;
        res.buildForPath(view, config);
        client.setResponseBuffer(res.getRaw());
        armClientWrite(client, epollFd);
    }
}

void Server::handleCgiPipeEvent(int pipeFd, uint32_t events, int epollFd) {
    auto mapIt = cgiPipeToClient_.find(pipeFd);
    if (mapIt == cgiPipeToClient_.end())
        return;
    auto clientIt = clients_.find(mapIt->second);
    if (clientIt == clients_.end() || clientIt->second->cgi() == nullptr) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, pipeFd, nullptr);
        cgiPipeToClient_.erase(mapIt);
        return;
    }

    Client& client = *(clientIt->second);
    CgiHandler& cgi = *client.cgi();
    if ((events & EPOLLOUT) && pipeFd == cgi.stdinFd())
        cgi.onStdinReady();
    if ((events & (EPOLLIN | EPOLLHUP | EPOLLERR)) && pipeFd == cgi.stdoutFd())
        cgi.onStdoutReady();

    syncCgiEpoll(client, epollFd);
    if (cgi.isFinished())
        finishCgi(client, epollFd);
}

void Server::checkCgiTimeouts(int epollFd) {
    for (auto& entry : clients_) {
        Client& client = *entry.second;
        if (client.getState() != ClientState::CGI_RUNNING || client.cgi() == nullptr)
            continue;
        client.cgi()->checkTimeout();
        if (client.cgi()->isFinished())
            finishCgi(client, epollFd);
        else
            syncCgiEpoll(client, epollFd);
    }
}

void Server::handleClientWrite(int clientFd, int epollFd,
                               std::map<int, std::unique_ptr<Client>>::iterator it) {
    Client& client = *(it->second);
    const std::string& response = client.getResponseBuffer();
    if (client.getOffset() >= response.size()) {
        closeClient(it, epollFd);
        return;
    }

    const size_t remaining = response.size() - client.getOffset();
    const ssize_t sent = send(clientFd, response.c_str() + client.getOffset(), remaining, 0);
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        closeClient(it, epollFd);
        return;
    }
    if (sent == 0) {
        closeClient(it, epollFd);
        return;
    }

    client.addOffset(static_cast<size_t>(sent));
    if (client.getOffset() >= response.size())
        closeClient(it, epollFd);
}
