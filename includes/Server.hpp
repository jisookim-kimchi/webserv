#pragma once

#include "Client.hpp"
#include "ListenSocket.hpp"
#include "ServerConfig.hpp"

#include <ctime>
#include <map>
#include <memory>
#include <sys/epoll.h>
#include <vector>

class HTTP_Request;

class Server {
   public:
    ~Server();
    explicit Server(const std::vector<ServerConfig>& configs);

    void run();

   private:
    const Server& operator=(const Server& other);
    Server(const Server& other);

    std::vector<std::unique_ptr<ListenSocket>> listenSockets_;
    std::vector<ServerConfig> serverConfigs_;
    std::map<int, std::unique_ptr<Client>> clients_;
    std::map<int, int> cgiPipeToClient_;  // pipe fd → client fd

    bool isListenSocket(int fd);
    bool hasActiveCgi() const;
    void armClientWrite(Client& client, int epollFd);
    void closeClient(std::map<int, std::unique_ptr<Client>>::iterator it, int epollFd);

    void handleNewConnection(int listenFd, int epollFd);
    void handleClientRead(int clientFd, int epollFd,
                          std::map<int, std::unique_ptr<Client>>::iterator it);
    void handleClientWrite(int clientFd, int epollFd,
                           std::map<int, std::unique_ptr<Client>>::iterator it);
    void processRequest(Client& client, int epollFd);

    void handleCgi(Client& client, const HTTP_Request& req, const LocationConfig* loc, int epollFd);
    void handleCgiPipeEvent(int pipeFd, uint32_t events, int epollFd);
    void syncCgiEpoll(Client& client, int epollFd);
    void finishCgi(Client& client, int epollFd);
    void checkCgiTimeouts(int epollFd);
    void checkIdleClients(int epollFd);

    const ServerConfig& findServerConfig(const Client& client, const HTTP_Request& req) const;
    CgiHandler::Request createCgiRequest(Client& client, const HTTP_Request& req,
                                         const LocationConfig* loc) const;
};
