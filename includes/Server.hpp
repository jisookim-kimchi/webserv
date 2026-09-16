#pragma once

#include "Client.hpp"
#include "ListenSocket.hpp"
#include "ServerConfig.hpp"
#include <map>
#include <sys/epoll.h>
#include <vector>
#include <memory>

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
};
