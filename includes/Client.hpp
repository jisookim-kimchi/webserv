#pragma once

#include "CgiHandler.hpp"
#include "HttpRequest.hpp"

#include <ctime>
#include <memory>
#include <netinet/in.h>
#include <string>

// Per-connection lifecycle driven by Server's epoll loop.
enum class ClientState {
    READING_REQUEST,
    CGI_RUNNING,
    WRITING_RESPONSE
};

class Client {
   public:
    Client(int fd, const struct sockaddr_in& addr, int serverPort, const std::string& serverHost);
    ~Client();

    Client(const Client& other) = delete;
    Client& operator=(const Client& other) = delete;

    int getFd() const;
    int getServerPort() const;
    const std::string& getServerHost() const;  // bind address of the accepting listen socket
    std::string getRemoteAddr() const;         // peer IPv4 for CGI REMOTE_ADDR

    ClientState getState() const;
    void setState(ClientState state);
    size_t getOffset() const { return offset_; }
    void addOffset(size_t bytes) { offset_ += bytes; }

    time_t getLastActiveTime() const;
    void updateLastActiveTime();

    const std::string& getRequestBuffer() const;
    std::string& getRequestBuffer();
    void appendRequestBuffer(const char* data, size_t size);

    const std::string& getResponseBuffer() const;
    void setResponseBuffer(std::string response);

    CgiHandler* cgi() { return cgi_.get(); }
    void setCgi(std::unique_ptr<CgiHandler> cgi) { cgi_ = std::move(cgi); }
    void clearCgi() { cgi_.reset(); }

    bool omitCgiBody() const { return omitCgiBody_; }
    void setOmitCgiBody(bool omit) { omitCgiBody_ = omit; }

    // Parked request while waiting for a free CGI slot (buffer already consumed).
    bool hasPendingRequest() const { return pendingRequest_ != nullptr; }
    void setPendingRequest(std::unique_ptr<HttpRequest> req) { pendingRequest_ = std::move(req); }
    std::unique_ptr<HttpRequest> takePendingRequest() { return std::move(pendingRequest_); }

   private:
    size_t offset_ = 0;
    int fd_;
    struct sockaddr_in addr_;
    int serverPort_;
    std::string serverHost_;

    std::string requestBuffer_;
    std::string responseBuffer_;

    ClientState state_;
    time_t lastActiveTime_;
    std::unique_ptr<CgiHandler> cgi_;
    bool omitCgiBody_ = false;
    std::unique_ptr<HttpRequest> pendingRequest_;
};
