#pragma once

#include <string>
#include <map>
#include "Utils.hpp"

enum class HttpMethod : unsigned int
{
    UNKNOWN = 0,
    GET     = 1 << 0,
    POST    = 1 << 1,
    DELETE  = 1 << 2,
    PUT     = 1 << 3,
    HEAD    = 1 << 4
};

class HttpRequest
{
public:
    HttpRequest() : method_(HttpMethod::UNKNOWN){}
    ~HttpRequest() = default;
    HttpRequest(const HttpRequest& other) = default;
    bool parse(const std::string& buffer);
    /*
        Getter
    */
    HttpMethod getMethod() const {return method_;}
    std::string getMethodString() const;
    const std::string& getPath() const { return path_; }
    const std::string& getQueryString() const { return queryString_; }
    const std::string& getVersion() const { return version_; }
    const std::string& getBody() const { return body_; }
    std::string& getBody() { return body_; }
    std::string getHeader(const std::string& key) const;
    const std::map<std::string, std::string>& getHeaders() const { return headers_; }

private:
    HttpRequest& operator=(const HttpRequest& other) = default;

    bool parseRequestLine(const std::string& buffer, size_t& headerStart);
    bool parseHeaders(const std::string& buffer, size_t headerStart, size_t headerEnd);
    bool parseBody(const std::string& buffer, size_t headerEnd);

    HttpMethod method_;
    std::string uri_;
    std::string version_;
    std::string path_;
    std::string queryString_;
    std::string body_;
    std::map<std::string, std::string> headers_;
};