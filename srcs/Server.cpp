#include "../includes/Server.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

constexpr int MAX_EVENTS = 256;
constexpr size_t BUF_SIZE = 4096;

Server::~Server() {
}

/*
    @brief assign operator disabled to prevent duplicating server instance.
*/
const Server& Server::operator=(const Server& other) {
    (void)other;
    return *this;
}

/*
    @brief Copy constructor disabled to prevent duplicating server instance.
*/
Server::Server(const Server& other) {
    (void)other;
}

/*
    @brief make socket for each ServerConfig and assign to listenSockets_ vector.
    @param serverConfigs Vector of ServerConfig objects.
*/
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
        if (listenSocket->getFd() == fd) {
            return true;
        }
    }
    return false;
}

/*
    @brief run Server...
*/
void Server::run() {
    int epollFd = epoll_create(1);  // checkpoint installed...
    if (epollFd == -1) {
        throw std::runtime_error("epoll created failed");
    }
    for (const auto& listenSocket : listenSockets_) {
        int fd = listenSocket->getFd();
        struct epoll_event event {};
        event.events = EPOLLIN;
        event.data.fd = fd;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
            throw std::runtime_error("epoll ctl add failed");
        }
    }
    struct epoll_event events[MAX_EVENTS]{};
    while (true) {
        int eventsFds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (eventsFds == -1) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("epoll wait failed");
        }
        for (int i = 0; i < eventsFds; i++) {
            int eventfd = events[i].data.fd;
            if (isListenSocket(eventfd)) {
                struct sockaddr_in clientAddr {};
                socklen_t clientAddrLen = sizeof(clientAddr);

                int clientFd = accept(eventfd, reinterpret_cast<struct sockaddr*>(&clientAddr),
                                      &clientAddrLen);
                if (clientFd != -1) {
                    int flags = fcntl(clientFd, F_GETFL, 0);
                    if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
                        close(clientFd);
                        continue;
                    }
                    int port = 0;
                    for (const auto& ls : listenSockets_) {
                        if (ls->getFd() == eventfd) {
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
            } else {
                auto it = clients_.find(eventfd);
                if (it == clients_.end())
                    continue;
                Client& client = *(it->second);

                if (events[i].events & EPOLLIN) {
                    char buf[BUF_SIZE];
                    ssize_t ret = read(eventfd, buf, BUF_SIZE);
                    if (ret <= 0) {
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, eventfd, nullptr);
                        clients_.erase(it);
                    } else {
                        client.appendRequestBuffer(buf, ret);
                        if (client.getRequestBuffer().find("\r\n\r\n") != std::string::npos) {
                            HTTP_Request req;
                            if (req.parse(client.getRequestBuffer()))
                            {
                                //TODO:generate HTTP response
                            }
                            else
                            {
                                //TODO: generate HTTP 400 Bad Response
                            }
                            client.setResponseBuffer(response);
                            client.setState(ClientState::WRITING_RESPONSE);
                            struct epoll_event ev {};
                            ev.events = EPOLLOUT;
                            ev.data.fd = eventfd;
                            epoll_ctl(epollFd, EPOLL_CTL_MOD, eventfd, &ev);
                        }
                    }
                } else if (events[i].events & EPOLLOUT) {
                    const std::string& response = client.getResponseBuffer();
                    ssize_t sent = send(eventfd, response.c_str(), response.length(), 0);
                    if (sent <= 0) {
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, eventfd, nullptr);
                        clients_.erase(it);
                    } else {
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, eventfd, nullptr);
                        clients_.erase(it);
                    }
                }
            }
        }
    }
}
