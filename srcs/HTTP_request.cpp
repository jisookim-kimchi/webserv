#include "HTTP_Request.hpp"

/**
 * @brief fetches header value by key.
 * @param key The header field name.
 * @return The header value, or empty string if not found.
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

/**
 * @brief  convert HTTP_METHOD enum value to string.
 * @return String representation of the method.
 */
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

/**
 * @brief parses HTTP method token from buffer to HTTP_METHOD enum.
 * @param buffer raw request buffer.
 * @param len Length of the method token.
 * @return HTTP_METHOD enum value, or UNKNOWN if not supported.
 */
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

/**
 * @brief converts a hex char to int value.
 * @param c hex char.
 * @param digit int value.
 * @return true if valid hex char, false otherwise.
 */
static bool hexCharToInt(char c, size_t &digit)
{
    if (c >= '0' && c <= '9')
    {
        digit = c - '0';
        return true;
    }
    char lower = c | 0x20;
    if (lower >= 'a' && lower <= 'f')
    {
        digit = lower - 'a' + 10;
        return true;
    }
    return false;
}

/**
 * @brief converts a hex string range to size_t integer.
 * @param str source string containing hex numbers.
 * @param pos starting offset of hex string.
 * @param len length of hex string.
 * @param outSize size_t integer.
 * @return true if valid hex number, false otherwise.
 */
static bool hexStrRangeToSize(const std::string &str, size_t pos, size_t len, size_t &outSize)
{
    if (len == 0 || pos + len > str.size())
        return false;
    outSize = 0;
    for (size_t i = 0; i < len; ++i)
    {
        size_t digit = 0;
        if (!hexCharToInt(str[pos + i], digit))
            return false;
        outSize = (outSize << 4) | digit;
    }
    return true;
}

/**
 * @brief decodes percent-encoded characters (%...) in URI path.
 * @param src source string containing encoded URI.
 * @param pos starting offset of URI path.
 * @param len length of URI path.
 * @param out reference to string where decoded path will be stored.
 * @return true if decoding succeeded, if not, failed.
 */
static bool urlDecode(const std::string &src, size_t pos, size_t len, std::string &out)
{
    out.clear();
    out.reserve(len);
    for (size_t i = 0; i < len; ++i)
    {
        size_t cur = pos + i;
        if (src[cur] == '%')
        {
            if (i + 2 >= len)
                return false;
            size_t high = 0, low = 0;
            if (!hexCharToInt(src[cur + 1], high) || !hexCharToInt(src[cur + 2], low))
                return false;
            out += static_cast<char>((high << 4) | low);
            i += 2;
        }
        else
        {
            out += src[cur];
        }
    }
    return true;
}

/**
 * @brief parses HTTP Request Line (Method, URI/Path/Query, Version).
 * @param buffer raw request buffer.
 * @param headerStart output parameter updated to the byte offset where headers begin.
 * @return true if Request-Line is valid HTTP/1.1, if not, failed.
 */
bool HTTP_Request::parseRequestLine(const std::string &buffer, size_t &headerStart)
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
    size_t uriLen = secondSpace - firstSpace - 1;
    uri_.assign(buffer, firstSpace + 1, uriLen);
    if (uri_.empty() || uri_[0] != '/')
        return false;
    size_t queryPos = uri_.find('?');
    size_t pathLen = queryPos != std::string::npos ? queryPos : uri_.size();
    if (queryPos != std::string::npos)
        queryString_.assign(uri_, queryPos + 1, uri_.size() - (queryPos + 1));
    else
        queryString_.clear();
    if (!urlDecode(uri_, 0, pathLen, path_))
        return false;
    size_t verLen = find_r_n - secondSpace - 1;
    if (buffer.compare(secondSpace + 1, verLen, "HTTP/1.1") != 0)
        return false;
    version_ = "HTTP/1.1";

    headerStart = find_r_n + 2;
    return true;
}

/**
 * @brief parses HTTP headers into map and validates mandatory headers (Host).
 * @param buffer raw request buffer.
 * @param headerStart starting byte offset of headers section.
 * @param headerEnd byte offset where header section ends ("\r\n\r\n").
 * @return true if all headers are valid, if not, failed.
 */
bool HTTP_Request::parseHeaders(const std::string &buffer, size_t headerStart, size_t headerEnd)
{
    bool hasHost = false;
    std::string key;
    std::string val;

    while (headerStart < headerEnd)
    {
        size_t rn = buffer.find("\r\n", headerStart);
        if (rn == std::string::npos || rn > headerEnd)
            return false;

        size_t colon = buffer.find(':', headerStart);
        if (colon == std::string::npos || colon > rn)
            return false;

        size_t keyLen = colon - headerStart;
        key.resize(keyLen);
        for (size_t i = 0; i < keyLen; ++i)
        {
            char c = buffer[headerStart + i];
            if (c >= 'A' && c <= 'Z')
                c |= 0x20;
            key[i] = c;
        }
        size_t valStart = colon + 1;
        while (valStart < rn && (buffer[valStart] == ' ' || buffer[valStart] == '\t'))
            valStart++;
        size_t valLen = rn - valStart;
        val.assign(buffer, valStart, valLen);
        if(key == "host")
        {
            if (hasHost)
                return false;
            hasHost = true;
        }
        headers_[key] = val;
        headerStart = rn + 2;
    }
    return hasHost;
}

/**
 * @brief parses HTTP Body handling Content-Length and Chunked Transfer-Encoding.
 * @param buffer raw request buffer.
 * @param headerEnd byte offset where headers end ("\r\n\r\n").
 * @return true if body was successfully read and validated, if not, failed.
 */
bool HTTP_Request::parseBody(const std::string &buffer, size_t headerEnd)
{
    std::map<std::string, std::string>::const_iterator conLenIt = headers_.find("content-length");
    bool hasContentLength = (conLenIt != headers_.end());
    bool isChunked = (getHeader("transfer-encoding") == "chunked");

    if (hasContentLength && isChunked)
        return false;

    const size_t bodyStart = headerEnd + 4;

    if (isChunked)
    {
        body_.clear();
        size_t pos = bodyStart;
        while (pos < buffer.size())
        {
            size_t rn = buffer.find("\r\n", pos);
            if (rn == std::string::npos)
                return false;
            size_t chunkSize = 0;
            if (!hexStrRangeToSize(buffer, pos, rn - pos, chunkSize))
                return false;
            if (chunkSize == 0)
            {
                // Need final CRLF after the 0-size chunk line (`0\r\n\r\n`).
                if (rn + 4 > buffer.size())
                    return false;
                return true;
            }
            size_t dataStart = rn + 2;
            if (dataStart + chunkSize + 2 > buffer.size())
                return false;
            body_.append(buffer, dataStart, chunkSize);
            pos = dataStart + chunkSize + 2;
        }
        return false;
    }

    if (hasContentLength)
    {
        const std::string& realLenStr = conLenIt->second;
        if (realLenStr.empty())
            return false;
        size_t expectedLen = 0;
        for (size_t i = 0; i < realLenStr.size(); i++)
        {
            if (!isdigit(realLenStr[i]))
                return false;
            expectedLen = (expectedLen << 3) + (expectedLen << 1) + (realLenStr[i] - '0');
        }
        const size_t available = buffer.size() - bodyStart;
        if (available < expectedLen)
            return false;
        body_.assign(buffer, bodyStart, expectedLen);
        return true;
    }

    // No Content-Length and not chunked: empty body (do not swallow trailing bytes).
    body_.clear();
    return true;
}

/**
 * @brief top-level request parser called Request Line, Headers, and Body parsing.
 * @param buffer complete raw HTTP request string.
 * @return true if parsing succeeded,if not, failed.
 */
bool HTTP_Request::parse(const std::string &buffer)
{
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    size_t headerStart = 0;
    if (!parseRequestLine(buffer, headerStart))
        return false;

    if (!parseHeaders(buffer, headerStart, headerEnd))
        return false;

    if (!parseBody(buffer, headerEnd))
        return false;

    return true;
}

