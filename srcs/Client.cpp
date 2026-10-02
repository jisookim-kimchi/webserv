#include "../includes/Client.hpp"

#include <unistd.h>

Client::Client(int fd, const struct sockaddr_in& addr, int serverPort)
    : fd_(fd),
      addr_(addr),
      serverPort_(serverPort),
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

const struct sockaddr_in& Client::getAddr() const {
    return addr_;
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

std::string& Client::getResponseBuffer() {
    return responseBuffer_;
}

void Client::setResponseBuffer(const std::string& response) {
    responseBuffer_ = response;
    offset_ = 0;
    updateLastActiveTime();
}
