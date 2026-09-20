#pragma once

#include "Client.hpp"
#include "ListenSocket.hpp"
#include "ServerConfig.hpp"
#include <map>
#include <sys/epoll.h>
#include <vector>
#include <memory>

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
    bool isListenSocket(int fd);
    void handleNewConnection(int listenFd, int epollFd);
    void handleClientRead(int clientFd, int epollFd, std::map<int, std::unique_ptr<Client>>::iterator it);
    void handleClientWrite(int clientFd, int epollFd, std::map<int, std::unique_ptr<Client>>::iterator it);
    void processRequest(Client& client, int epollFd);
    void handleCgi(Client& client, const HTTP_Request& req, const LocationConfig* loc);
    const ServerConfig& findServerConfig(const Client& client, const HTTP_Request& req) const;
};
