
#include "../includes/Server.hpp"

Server::Server()
{

}

Server::~Server()
{

}

/*
    @brief assign operator disabled to prevent duplicating server instance.
*/
const Server &Server::operator=(const Server &other)
{
    (void)other;
    return *this;
}

/*
    @brief Copy constructor disabled to prevent duplicating server instance.
*/
Server::Server(const Server &other)
{
    (void)other;
}

/*
    @brief make socket for each ServerConfig and assign to listenSockets_ vector.
    @param serverConfigs Vector of ServerConfig objects.
*/
Server::Server(const std::vector<ServerConfig> &serverConfigs) : serverConfigs_(serverConfigs)
{
    for (size_t i = 0; i < serverConfigs_.size(); i++)
    {
        const std::string &host = serverConfigs_[i].getHost();
        const std::vector<uint16_t> &ports = serverConfigs_[i].getPort();
        for (size_t j = 0; j < ports.size(); j++)
        {
            std::unique_ptr<ListenSocket> socket = std::make_unique<ListenSocket>();
            socket->createSocket();
            socket->setSocketOption();
            socket->setNonBlocking();
            socket->bind(ports[j], host); 
            socket->listen();
            listenSockets_.push_back(std::move(socket));
        }
    }
}

/*
    @brief run Server...  
*/
void Server::run()
{

}