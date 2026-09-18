#pragma once

#include <string>
#include <map>
#include "Utils.hpp"

enum class HTTP_METHOD : unsigned int
{
    UNKNOWN = 0,
    GET     = 1 << 0,
    POST    = 1 << 1,
    DELETE  = 1 << 2,
    HEAD    = 1 << 3,
    PUT     = 1 << 4,
    PATCH   = 1 << 5,
    OPTIONS = 1 << 6,
    TRACE   = 1 << 7,
    CONNECT = 1 << 8
};

class HTTP_Request
{
public:
    HTTP_Request() : method_(HTTP_METHOD::UNKNOWN){}
    ~HTTP_Request() = default;
    HTTP_Request(const HTTP_Request& other) = default;
    bool parse(const std::string& buffer);
    /*
        Getter
    */
    HTTP_METHOD getMethod() const {return method_;}
    std::string getMethodString() const;
    const std::string& getPath() const { return path_; }
    const std::string& getQueryString() const { return queryString_; }
    const std::string& getVersion() const { return version_; }
    const std::string& getBody() const { return body_; }
    std::string getHeader(const std::string& key) const;
    const std::map<std::string, std::string>& getHeaders() const { return headers_; }

private:
    HTTP_Request& operator=(const HTTP_Request& other) = default;

    bool parseRequestLine(const std::string& buffer, size_t& headerStart);
    bool parseHeaders(const std::string& buffer, size_t headerStart, size_t headerEnd);
    bool parseBody(const std::string& buffer, size_t headerEnd);

    HTTP_METHOD method_;
    std::string uri_;
    std::string version_;
    std::string path_;
    std::string queryString_;
    std::string body_;
    std::map<std::string, std::string> headers_;
};