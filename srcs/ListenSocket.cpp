#include "../includes/ListenSocket.hpp"
#include "../includes/Utils.hpp"

/*
    @brief Socket create
    @Socket domain = AF_INET (IPv4)
    @Socket type = SOCK_STREAM (TCP)
    @Socket protocol = 0 (OS chooses default protocol)
    @throws std::runtime_error if socket creation fails
*/
void ListenSocket::createSocket() {
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ == -1)
        throw std::runtime_error("Error : ListenSocket::createSocket()");
}

/*
    @brief set socket option
    @param opt = 1 (enable option)
    @throws std::runtime_error if setsockopt fails
*/
void ListenSocket::setSocketOption() {
    int opt = 1;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("Error : ListenSocket::setSocketOption()");
}

/*
    @brief set socket non-blocking mode
    @throws std::runtime_error if fcntl fails
*/
void ListenSocket::setNonBlocking() {
    try {
        Utils::setNonBlocking(fd_);
    } catch (const std::exception&) {
        throw std::runtime_error("Error : ListenSocket::setNonBlocking()");
    }
}

/*
    @brief bind Socket port and host(IP address)
    @param1 port number
    @param2 host(IP address) if empty, use DEFAULT_HOST(INADDR_ANY)
*/
void ListenSocket::bind(int port, const std::string& host) {
    port_ = port;
    host_ = (host.empty() ? DEFAULT_HOST : host);

    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    const char* node = (host.empty() || host == DEFAULT_HOST) ? nullptr : host.c_str();
    const std::string service = std::to_string(port_);
    struct addrinfo* result = nullptr;
    const int gai = ::getaddrinfo(node, service.c_str(), &hints, &result);
    if (gai != 0 || result == nullptr) {
        throw std::runtime_error(std::string("Error : ListenSocket::bind(): ") +
                                 ::gai_strerror(gai));
    }

    if (result->ai_addr == nullptr || result->ai_addrlen < sizeof(sockaddr_in)) {
        ::freeaddrinfo(result);
        throw std::runtime_error("Error : ListenSocket::bind()");
    }

    addr_ = *reinterpret_cast<sockaddr_in*>(result->ai_addr);
    ::freeaddrinfo(result);

    if (::bind(fd_, reinterpret_cast<struct sockaddr*>(&addr_), sizeof(addr_)) < 0)
        throw std::runtime_error("Error : ListenSocket::bind()");
}

/*
    @brief listen socket
    @param backlog maximum number of pending connections, SOMAXCONN is Max size of pending
   connections in OS
    @throws std::runtime_error if listen fails
*/
void ListenSocket::listen() {
    if (::listen(fd_, SOMAXCONN) < 0)
        throw std::runtime_error("Error : ListenSocket::listen()");
}

/*
    @brief close socket
*/
void ListenSocket::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}
