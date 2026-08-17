#pragma once

#include "ListenSocket.hpp"
#include "ServerConfig.hpp"
#include <vector>
#include <memory>

class Server
{
public:
    Server();
    ~Server();
    explicit Server(const std::vector<ServerConfig> &configs);

    void run();
    
private:
    const Server &operator=(const Server &other);
    Server(const Server &other);

    std::vector<std::unique_ptr<ListenSocket>> listenSockets_;
    std::vector<ServerConfig> serverConfigs_;
};