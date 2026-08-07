#pragma once

#include <sys/socket.h>
#include <netline/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>

const std::string DEFAULT_HOST = "0.0.0.0";

class ListenSocket
{
public:
	ListenSocket() : _fd(-1), _port(-1), _addr{} {}
	~ListenSocket() { this->close(); }

    void createSocket();
    void setSocketOption();
    void bind(int port, std::string& host);
    void listen();
    int accept(struct sockaddr_in &clientAddr);
    
    void close();
    const int &getFd() const { return _fd; }
    const int &getPort() const { return _port; }
    const struct sockaddr_in &getAddr() const { return _addr; }

private:
    ListenSocket(const ListenSocket& other);
    ListenSocket& operator=(const ListenSocket& other);
    
	int _fd;
    struct sockaddr_in _addr;
    int _port;

};