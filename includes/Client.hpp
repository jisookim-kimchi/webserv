#pragma once
/*
client's are made only when server accept's
*/
#include "CgiHandler.hpp"

#include <ctime>
#include <memory>
#include <netinet/in.h>
#include <string>

enum class ClientState {
    READING_REQUEST,
    CGI_RUNNING,
    WRITING_RESPONSE,
    FINISHED
};

class Client {
   public:
    Client(int fd, const struct sockaddr_in& addr, int serverPort);
    ~Client();

    int getFd() const;
    int getServerPort() const;
    const struct sockaddr_in& getAddr() const;

    ClientState getState() const;
    void setState(ClientState state);
    size_t getOffset() const { return offset_; }
    void addOffset(size_t bytes) { offset_ += bytes; }
    void resetOffset() { offset_ = 0; }

    time_t getLastActiveTime() const;
    void updateLastActiveTime();

    const std::string& getRequestBuffer() const;
    std::string& getRequestBuffer();
    void appendRequestBuffer(const char* data, size_t size);
    void clearRequestBuffer();

    const std::string& getResponseBuffer() const;
    std::string& getResponseBuffer();
    void setResponseBuffer(const std::string& response);
    void appendResponseBuffer(const char* data, size_t size);
    void clearResponseBuffer();

    CgiHandler* cgi() { return cgi_.get(); }
    void setCgi(std::unique_ptr<CgiHandler> cgi) { cgi_ = std::move(cgi); }
    void clearCgi() { cgi_.reset(); }

   private:
    const Client& operator=(const Client& other) = delete;
    Client(const Client& other) = delete;

    size_t offset_ = 0;
    int fd_;
    struct sockaddr_in addr_;
    int serverPort_;

    std::string requestBuffer_;
    std::string responseBuffer_;

    ClientState state_;
    time_t lastActiveTime_;
    std::unique_ptr<CgiHandler> cgi_;
};
