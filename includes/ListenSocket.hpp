#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <stdexcept>

const std::string DEFAULT_HOST = "0.0.0.0";

class ListenSocket
{
public:
	ListenSocket() : fd_(-1), port_(-1), addr_{} {}
	~ListenSocket() { this->close(); }

    void createSocket();
    void setSocketOption();
    void setNonBlocking();
    void bind(int port, const std::string& host);
    void listen();
    int accept(struct sockaddr_in &clientAddr);
    
    void close();
    const int &getFd() const { return fd_; }
    const int &getPort() const { return port_; }
    const struct sockaddr_in &getAddr() const { return addr_; }

private:
    ListenSocket(const ListenSocket& other);
    ListenSocket& operator=(const ListenSocket& other);
    
	int fd_;
    int port_;
    struct sockaddr_in addr_;

};