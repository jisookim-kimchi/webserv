#include "HTTP_Request.hpp"

/*
    Getter
*/
std::string HTTP_Request::getHeader(const std::string& key) const
{
    std::string lowerKey = Utils::toLower(key);
    std::map<std::string, std::string>::const_iterator it = headers_.find(lowerKey);
    if (it != headers_.end())
        return it->second;
    return "";
}

std::string HTTP_Request::getMethodString() const
{
    if (method_ == HTTP_METHOD::GET)
        return "GET";
    else if (method_ == HTTP_METHOD::POST)
        return "POST";
    else if (method_ == HTTP_METHOD::DELETE)
        return "DELETE";
    else if (method_ == HTTP_METHOD::HEAD)
        return "HEAD";
    else if (method_ == HTTP_METHOD::PUT)
        return "PUT";
    else if (method_ == HTTP_METHOD::PATCH)
        return "PATCH";
    else if (method_ == HTTP_METHOD::OPTIONS)
        return "OPTIONS";
    else if (method_ == HTTP_METHOD::TRACE)
        return "TRACE";
    else if (method_ == HTTP_METHOD::CONNECT)
        return "CONNECT";
    return "UNKNOWN";
}

static HTTP_METHOD parseMethod(const std::string &methodStr)
{
    if (methodStr == "GET")
        return HTTP_METHOD::GET;
    else if (methodStr == "POST")
        return HTTP_METHOD::POST;
    else if (methodStr == "DELETE")
        return HTTP_METHOD::DELETE;
    else if (methodStr == "HEAD")
        return HTTP_METHOD::HEAD;
    else if (methodStr == "PUT")
        return HTTP_METHOD::PUT;
    else if (methodStr == "PATCH")
        return HTTP_METHOD::PATCH;
    else if (methodStr == "OPTIONS")
        return HTTP_METHOD::OPTIONS;
    else if (methodStr == "TRACE")
        return HTTP_METHOD::TRACE;
    else if (methodStr == "CONNECT")
        return HTTP_METHOD::CONNECT;
    return HTTP_METHOD::UNKNOWN;
}

bool HTTP_Request::parse(const std::string &buffer)
{

    size_t find_r_n = buffer.find("\r\n");
    if (find_r_n == std::string::npos)
        return false;
    size_t firstSpace = buffer.find(' ');
    if (firstSpace == std::string::npos)
        return false;
    std::string method = buffer.substr(0, firstSpace);
    method_ = parseMethod(method);
    if (method_ == HTTP_METHOD::UNKNOWN)
        return false;
    // /index.html HTTP/1.1
    size_t secondSpace = buffer.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
        return false;
    uri_ = buffer.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    if (uri_.empty() || uri_[0] != '/')
        return false;
    size_t queryPos = uri_.find('?');
    if (queryPos != std::string::npos)
    {
        path_ = uri_.substr(0, queryPos);
        queryString_ = uri_.substr(queryPos + 1);
    }
    else
    {
        path_ = uri_;
        queryString_.clear();
    }
    version_ = buffer.substr(secondSpace + 1, find_r_n - secondSpace - 1);
    if (version_ != "HTTP/1.1")
        return false;
    
    //parse headers
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;
    size_t headerStart = find_r_n + 2;
    bool hasHost = false;
    while (headerStart < headerEnd)
    {
        size_t rn = buffer.find("\r\n", headerStart);
        if (rn == std::string::npos)
            return false;
        std::string line = buffer.substr(headerStart, rn - headerStart);
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            return false;
        std::string key = Utils::toLower(line.substr(0, colon));
        std::string val = Utils::trimLeft(line.substr(colon + 1));
        if(key == "host")
        {
            if (hasHost)
                return false;
            hasHost = true;
        }
        headers_[key] = val;
        headerStart = rn + 2;
    }
    if (!hasHost)
        return false;
    body_ = buffer.substr(headerEnd + 4);
    return true;
}

