#pragma once
/*
client's are made only when server accept's
*/
#include <netinet/in.h>
#include <string>
#include <ctime>

enum class ClientState
{
    READING_REQUEST,
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

   private:
    const Client& operator=(const Client& other) = delete;
    Client(const Client& other) = delete;

    int fd_;
    struct sockaddr_in addr_;
    int serverPort_;

    std::string requestBuffer_;
    std::string responseBuffer_;

    ClientState state_;
    time_t lastActiveTime_;
};