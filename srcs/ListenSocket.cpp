#include "../includes/ListenSocket.hpp"

/*
    @brief Socket create
    @Socket domain = AF_INET (IPv4)
    @Socket type = SOCK_STREAM (TCP)
    @Socket protocol = 0 (OS chooses default protocol)
    @throws std::runtime_error if socket creation fails
*/
void ListenSocket::createSocket()
{
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ == -1)
        throw std::runtime_error("Error : ListenSocket::createSocket()");
}

/*
    @brief set socket option
    @param opt = 1 (enable option)
    @throws std::runtime_error if setsockopt fails
*/
void ListenSocket::setSocketOption()
{
    int opt = 1;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("Error : ListenSocket::setSocketOption()");
}

/*
    @brief set socket non-blocking mode
    @throws std::runtime_error if fcntl fails
*/
void ListenSocket::setNonBlocking()
{
    if (::fcntl(fd_, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("Error : ListenSocket::setNonBlocking()");
}

/*
    @brief bind Socket port and host(IP address)
    @param1 port number
    @param2 host(IP address) if empty, use DEFAULT_HOST(INADDR_ANY)
*/
void ListenSocket::bind(int port, const std::string& host)
{
    port_ = port;
    addr_.sin_family = AF_INET;
    if (host.empty() || host == DEFAULT_HOST)
        addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    else
        addr_.sin_addr.s_addr = inet_addr(host.c_str());
    addr_.sin_port = htons(port_);
    if (::bind(fd_, (struct sockaddr *)&addr_, sizeof(addr_)) < 0)
        throw std::runtime_error("Error : ListenSocket::bind()");
    else
        std::cout << "ListenSocket bind success : " << port_ << std::endl;
}

/*
    @brief listen socket
    @param backlog maximum number of pending connections, SOMAXCONN is Max size of pending connections in OS
    @throws std::runtime_error if listen fails
*/
void ListenSocket::listen()
{
    if (::listen(fd_, SOMAXCONN ) < 0)
        throw std::runtime_error("Error : ListenSocket::listen()");
    else
        std::cout << "ListenSocket listen success : " << port_ << std::endl;
}

/*
    @brief accept Connection from client
    @return ClientSocket
    @throws std::runtime_error if accept fails
*/
int ListenSocket::accept(struct sockaddr_in &clientAddr)
{
    socklen_t clientAddrLen = sizeof(clientAddr);
    int client_socket_fd = ::accept(fd_, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (client_socket_fd == -1)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return -1;
        throw std::runtime_error("Error : ListenSocket::accept()");
    }
    else
        std::cout << "ListenSocket::accept success : " << client_socket_fd << std::endl;
    return client_socket_fd;
}

/*
    @brief close socket
    @throws std::runtime_error if close fails
*/
void ListenSocket::close()
{
    if (fd_ != -1)
    {
        ::close(fd_);
        fd_ = -1;
        std::cout << "ListenSocket close success! (Port : " << port_ << ")" << std::endl;
    }
}

