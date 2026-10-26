#include "HttpRequest.hpp"

/**
 * @brief fetches header value by key.
 * @param key The header field name.
 * @return The header value, or empty string if not found.
 */
std::string HttpRequest::getHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = headers_.find(Utils::toLower(key));
    if (it != headers_.end())
        return it->second;
    return "";
}

/**
 * @brief  convert HttpMethod enum value to string.
 * @return String representation of the method.
 */
std::string HttpRequest::getMethodString() const
{
    switch (method_)
    {
        case HttpMethod::GET:
            return "GET";
        case HttpMethod::POST:
            return "POST";
        case HttpMethod::DELETE:
            return "DELETE";
        case HttpMethod::PUT:
            return "PUT";
        case HttpMethod::HEAD:
            return "HEAD";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief parses HTTP method token from buffer to HttpMethod enum.
 * @param buffer raw request buffer.
 * @param len Length of the method token.
 * @return HttpMethod enum value, or UNKNOWN if not supported.
 */
static HttpMethod parseMethod(const std::string &buffer, size_t len)
{
    if (buffer.compare(0, len, "GET") == 0)
        return HttpMethod::GET;
    if (buffer.compare(0, len, "POST") == 0)
        return HttpMethod::POST;
    if (buffer.compare(0, len, "DELETE") == 0)
        return HttpMethod::DELETE;
    if (buffer.compare(0, len, "PUT") == 0)
        return HttpMethod::PUT;
    if (buffer.compare(0, len, "HEAD") == 0)
        return HttpMethod::HEAD;
    return HttpMethod::UNKNOWN;
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
            if (!Utils::hexCharToInt(src[cur + 1], high) || !Utils::hexCharToInt(src[cur + 2], low))
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
bool HttpRequest::parseRequestLine(const std::string &buffer, size_t &headerStart)
{
    size_t find_r_n = buffer.find("\r\n");
    if (find_r_n == std::string::npos)
        return false;
    size_t firstSpace = buffer.find(' ');
    if (firstSpace == std::string::npos)
        return false;
    method_ = parseMethod(buffer, firstSpace);
    if (method_ == HttpMethod::UNKNOWN)
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
bool HttpRequest::parseHeaders(const std::string &buffer, size_t headerStart, size_t headerEnd)
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

        key = Utils::toLower(buffer.substr(headerStart, colon - headerStart));
        val = Utils::trimLeft(buffer.substr(colon + 1, rn - (colon + 1)));
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
bool HttpRequest::parseBody(std::string &buffer, size_t headerEnd)
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
            if (!Utils::parseHexSize(buffer, pos, rn - pos, chunkSize))
                return false;
            if (chunkSize == 0)
            {
                if (rn + 4 > buffer.size())
                    return false;
                buffer.clear();
                buffer.shrink_to_fit();
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
        size_t expectedLen = 0;
        if (!Utils::parseSizeDecimal(conLenIt->second, expectedLen))
            return false;
        if (buffer.size() < bodyStart || buffer.size() - bodyStart < expectedLen)
            return false;
        // Take ownership of the raw buffer, then drop the header prefix so the
        // decoded body and the wire copy are never both resident.
        body_.swap(buffer);
        body_.erase(0, bodyStart);
        body_.resize(expectedLen);
        body_.shrink_to_fit();
        return true;
    }

    body_.clear();
    buffer.clear();
    buffer.shrink_to_fit();
    return true;
}

/**
 * @brief top-level request parser called Request Line, Headers, and Body parsing.
 * @param buffer complete raw HTTP request string; cleared on successful parse.
 * @return true if parsing succeeded,if not, failed.
 */
bool HttpRequest::parse(std::string &buffer)
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

