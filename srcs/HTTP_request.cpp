#include "HTTP_Request.hpp"

/*
    Getter
*/
std::string HTTP_Request::getHeader(const std::string& key) const
{
    std::string lowerKey = key;
    for (size_t i = 0; i < lowerKey.size(); ++i)
    {
        if (lowerKey[i] >= 'A' && lowerKey[i] <= 'Z')
            lowerKey[i] |= 0x20;
    }
    std::map<std::string, std::string>::const_iterator it = headers_.find(lowerKey);
    if (it != headers_.end())
        return it->second;
    return "";
}

std::string HTTP_Request::getMethodString() const
{
    switch (method_)
    {
        case HTTP_METHOD::GET:
            return "GET";
        case HTTP_METHOD::POST:
            return "POST";
        case HTTP_METHOD::DELETE:
            return "DELETE";
        case HTTP_METHOD::HEAD:
            return "HEAD";
        case HTTP_METHOD::PUT:
            return "PUT";
        case HTTP_METHOD::PATCH:
            return "PATCH";
        case HTTP_METHOD::OPTIONS:
            return "OPTIONS";
        case HTTP_METHOD::TRACE:
            return "TRACE";
        case HTTP_METHOD::CONNECT:
            return "CONNECT";
        default:
            return "UNKNOWN";
    }
}

static HTTP_METHOD parseMethod(const std::string &buffer, size_t len)
{
    if (buffer.compare(0, len, "GET") == 0)
        return HTTP_METHOD::GET;
    else if (buffer.compare(0, len, "POST") == 0)
        return HTTP_METHOD::POST;
    else if (buffer.compare(0, len, "DELETE") == 0)
        return HTTP_METHOD::DELETE;
    else if (buffer.compare(0, len, "HEAD") == 0)
        return HTTP_METHOD::HEAD;
    else if (buffer.compare(0, len, "PUT") == 0)
        return HTTP_METHOD::PUT;
    else if (buffer.compare(0, len, "PATCH") == 0)
        return HTTP_METHOD::PATCH;
    else if (buffer.compare(0, len, "OPTIONS") == 0)
        return HTTP_METHOD::OPTIONS;
    else if (buffer.compare(0, len, "TRACE") == 0)
        return HTTP_METHOD::TRACE;
    else if (buffer.compare(0, len, "CONNECT") == 0)
        return HTTP_METHOD::CONNECT;
    else
        return HTTP_METHOD::UNKNOWN;
}

/*
    @brief : to check Chunk Size range is `0~f`
*/
static bool hexStringToInt(const std::string &str, size_t &outSize)
{
    if (str.empty())
        return false;
    outSize = 0;
    for (size_t i = 0; i < str.size(); ++i)
    {
        char c = str[i];
        size_t digit = 0;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else
        {
            char lower = c | 0x20;
            if (lower >= 'a' && lower <= 'f')
                digit = lower - 'a' + 10;
            else
                return false;
        }
        outSize = (outSize << 4) | digit;
    }
    return true;
}

bool HTTP_Request::parse(const std::string &buffer)
{
    size_t find_r_n = buffer.find("\r\n");
    if (find_r_n == std::string::npos)
        return false;
    size_t firstSpace = buffer.find(' ');
    if (firstSpace == std::string::npos)
        return false;
    method_ = parseMethod(buffer, firstSpace);
    if (method_ == HTTP_METHOD::UNKNOWN)
        return false;
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
    size_t verLen = find_r_n - secondSpace - 1;
    if (buffer.compare(secondSpace + 1, verLen, "HTTP/1.1") != 0)
        return false;
    version_ = "HTTP/1.1";
    
    //parse headers
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;
    size_t headerStart = find_r_n + 2;
    bool hasHost = false;
    while (headerStart < headerEnd)
    {
        size_t rn = buffer.find("\r\n", headerStart);
        if (rn == std::string::npos || rn > headerEnd)
            return false;

        size_t colon = buffer.find(':', headerStart);
        if (colon == std::string::npos || colon > rn)
            return false;

        std::string key = buffer.substr(headerStart, colon - headerStart);
        for (size_t i = 0; i < key.size(); ++i)
        {
            if (key[i] >= 'A' && key[i] <= 'Z')
                key[i] |= 0x20;
        }
        size_t valStart = colon + 1;
        while (valStart < rn && (buffer[valStart] == ' ' || buffer[valStart] == '\t'))
            valStart++;
        std::string val = buffer.substr(valStart, rn - valStart);
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
    if (getHeader("transfer-encoding") == "chunked")
    {
        std::string rawBody = buffer.substr(headerEnd + 4);
        std::string mergedBody;
        size_t pos = 0;
        while (pos < rawBody.size())
        {
            size_t rn =rawBody.find("\r\n", pos);
            if (rn == std::string::npos)
                return false;
            std::string sizeStr = rawBody.substr(pos, rn - pos);
            size_t chunkSize = 0;
            if (!hexStringToInt(sizeStr, chunkSize))
                return false;
            if (chunkSize == 0)
                break;
            size_t dataStart = rn + 2;
            if (dataStart + chunkSize + 2 > rawBody.size())
                return false;
            mergedBody.append(rawBody, dataStart, chunkSize);
            pos = dataStart + chunkSize + 2;
        }
        body_ = mergedBody;
    }
    else
        body_ = buffer.substr(headerEnd + 4);
    return true;
}

