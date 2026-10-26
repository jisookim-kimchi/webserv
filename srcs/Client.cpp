#include "../includes/Client.hpp"

#include <arpa/inet.h>
#include <cstdio>
#include <unistd.h>

namespace {

std::string ipv4ToString(const struct in_addr& addr) {
    const unsigned long n = ntohl(addr.s_addr);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%lu.%lu.%lu.%lu", (n >> 24) & 0xffUL, (n >> 16) & 0xffUL,
                  (n >> 8) & 0xffUL, n & 0xffUL);
    return buf;
}

}  // namespace

Client::Client(int fd, const struct sockaddr_in& addr, int serverPort, const std::string& serverHost)
    : fd_(fd),
      addr_(addr),
      serverPort_(serverPort),
      serverHost_(serverHost),
      state_(ClientState::READING_REQUEST),
      lastActiveTime_(std::time(nullptr)) {}

Client::~Client() {
    cgi_.reset();
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}

int Client::getFd() const {
    return fd_;
}

int Client::getServerPort() const {
    return serverPort_;
}

const std::string& Client::getServerHost() const {
    return serverHost_;
}

std::string Client::getRemoteAddr() const {
    return ipv4ToString(addr_.sin_addr);
}

ClientState Client::getState() const {
    return state_;
}

void Client::setState(ClientState state) {
    state_ = state;
}

time_t Client::getLastActiveTime() const {
    return lastActiveTime_;
}

void Client::updateLastActiveTime() {
    lastActiveTime_ = std::time(nullptr);
}

const std::string& Client::getRequestBuffer() const {
    return requestBuffer_;
}

std::string& Client::getRequestBuffer() {
    return requestBuffer_;
}

void Client::appendRequestBuffer(const char* data, size_t size) {
    requestBuffer_.append(data, size);
    updateLastActiveTime();
}

const std::string& Client::getResponseBuffer() const {
    return responseBuffer_;
}

void Client::setResponseBuffer(std::string response) {
    responseBuffer_ = std::move(response);
    offset_ = 0;
    updateLastActiveTime();
}
