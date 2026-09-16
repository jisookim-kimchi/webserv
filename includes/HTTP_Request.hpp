#pragma once

#include <string>
#include <map>
#include "Utils.hpp"

enum class HTTP_METHOD
{
  GET,
  POST,
  DELETE,
  HEAD,
  PUT,
  PATCH,
  OPTIONS,
  TRACE,
  CONNECT,
  UNKNOWN,
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
    HTTP_METHOD method_;
    std::string uri_;
    std::string version_;
    std::string path_;
    std::string queryString_;
    std::string body_;
    std::map<std::string, std::string> headers_;
};