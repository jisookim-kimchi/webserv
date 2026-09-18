#pragma once

#include "ServerConfig.hpp"

#include <map>
#include <string>

namespace LocationMatch {

// Longest prefix match; extension locations (e.g. ".py") win when applicable.
const LocationConfig* match(const std::string& urlPath, const ServerConfig& server);

}  // namespace LocationMatch

// Builds HTTP/1.1 responses for an already-matched location (Overview).
// Does not route, parse requests, or run CGI.
class HttpResponse {
   public:
    struct RequestView {
        std::string method;
        std::string path;
        std::string body;
        int errorStatus = 0;  // parser / limit failure → emit this status
    };

    HttpResponse() = default;

    HttpResponse(const HttpResponse&) = delete;
    HttpResponse& operator=(const HttpResponse&) = delete;

    void reset();

    // `location` must already be selected by LocationMatch::match (or tests).
    void build(const RequestView& request, const ServerConfig& server,
               const LocationConfig& location);

    // Convenience: match then build. Returns false if no location matched
    // (response is already set to 404).
    bool buildForPath(const RequestView& request, const ServerConfig& server);

    int statusCode() const {
        return statusCode_;
    }
    const std::string& getRaw() const {
        return raw_;
    }
    const std::string& getBody() const {
        return body_;
    }
    const std::map<std::string, std::string>& getHeaders() const {
        return headers_;
    }
    bool needsCgi() const {
        return needsCgi_;
    }

    static std::string statusText(int code);

   private:
    bool methodAllowed(const std::string& method, const LocationConfig& loc) const;
    std::string mapUrlToFs(const LocationConfig& loc, const std::string& urlPath) const;

    void setError(int code, const ServerConfig& server, const LocationConfig* loc);
    void setRedirect(int code, const std::string& target);
    void setFile(const std::string& fsPath);
    void setAutoindex(const std::string& fsDir, const std::string& urlPath);
    void setEmpty(int code);
    void finalize();

    std::string loadErrorPage(int code, const ServerConfig& server,
                              const LocationConfig* loc) const;
    static std::string defaultErrorBody(int code);
    static std::string contentType(const std::string& path);
    static std::string joinPath(const std::string& a, const std::string& b);
    static bool isDir(const std::string& path);
    static bool isFile(const std::string& path);
    static bool readAll(const std::string& path, std::string& out);

    int statusCode_ = 0;
    std::map<std::string, std::string> headers_;
    std::string body_;
    std::string raw_;
    bool needsCgi_ = false;
};
