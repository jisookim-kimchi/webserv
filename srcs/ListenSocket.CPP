#include "../includes/ListenSocket.hpp"
#include <cerrno>

/*
    @brief Socket create
    @Socket domain = AF_INET (IPv4)
    @Socket type = SOCK_STREAM (TCP)
    @Socket protocol = 0 (OS chooses default protocol)
    @throws std::runtime_error if socket creation fails
*/
void ListenSocket::createSocket()
{
    _fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (_fd == -1)
        throw std::runtime_error("Error : createSocket()");
}

/*
    @brief set socket option
    @param opt = 1 (enable option)
    @throws std::runtime_error if setsockopt fails
*/
void ListenSocket::setSocketOption()
{
    int opt = 1;
    if (::setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("Error : setSocketOption()");
}

/*
    @brief set socket non-blocking mode
    @throws std::runtime_error if fcntl fails
*/
void ListenSocket::setNonBlocking()
{
    if (::fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("Error : setNonBlocking()");
}

/*
    @brief bind Socket port and host(IP address)
    @param1 port number
    @param2 host(IP address) if empty, use DEFAULT_HOST(INADDR_ANY)
*/
void ListenSocket::bind(int port, const std::string& host)
{
    _port = port;
    _addr.sin_family = AF_INET;
    if (host.empty() || host == DEFAULT_HOST)
        _addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else
        _addr.sin_addr.s_addr = inet_addr(host.c_str());
    _addr.sin_port = htons(_port);
    if (::bind(_fd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0)
        throw std::runtime_error("Error : bind()");
    else
        std::cout << "bind success : " << _port << std::endl;
}

/*
    @brief listen socket
    @param backlog maximum number of pending connections, SOMAXCONN is Max size of pending connections in OS
    @throws std::runtime_error if listen fails
*/
void ListenSocket::listen()
{
    if (::listen(_fd, SOMAXCONN ) < 0)
        throw std::runtime_error("Error : listen()");
    else
        std::cout << "listen success : " << _port << std::endl;
}

/*
    @brief accept Connection from client
    @return ClientSocket
    @throws std::runtime_error if accept fails
*/
int ListenSocket::accept(struct sockaddr_in &clientAddr)
{
    socklen_t clientAddrLen = sizeof(clientAddr);
    int client_socket_fd = ::accept(_fd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (client_socket_fd == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        throw std::runtime_error("Error : accept()");
    }
    std::cout << "accept success : " << client_socket_fd << std::endl;
    return client_socket_fd;
}

/*
    @brief close socket
    @throws std::runtime_error if close fails
*/
void ListenSocket::close()
{
    if (_fd != -1)
    {
        ::close(_fd);
        _fd = -1;
        std::cout << "Server close success! (Port : " << _port << ")" << std::endl;
    }
}

