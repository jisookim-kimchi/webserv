#include "../includes/HttpResponse.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>

namespace {

std::string toLower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string normalizePath(const std::string& path) {
    if (path.empty()) {
        return "/";
    }
    std::vector<std::string> parts;
    std::string cur;
    for (char c : path) {
        if (c == '/') {
            if (!cur.empty()) {
                parts.push_back(cur);
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) {
        parts.push_back(cur);
    }

    std::vector<std::string> out;
    for (const std::string& p : parts) {
        if (p == "." || p.empty()) {
            continue;
        }
        if (p == "..") {
            if (!out.empty()) {
                out.pop_back();
            }
            continue;
        }
        out.push_back(p);
    }

    std::string result = "/";
    for (std::size_t i = 0; i < out.size(); ++i) {
        if (i) {
            result.push_back('/');
        }
        result += out[i];
    }
    if (out.empty()) {
        return "/";
    }
    if (path.size() > 1 && path.back() == '/') {
        result.push_back('/');
    }
    return result;
}

}  // namespace

namespace LocationMatch {

const LocationConfig* match(const std::string& urlPath, const ServerConfig& server) {
    const std::string path = normalizePath(urlPath);
    const LocationConfig* best = nullptr;
    std::size_t bestLen = 0;
    const LocationConfig* ext = nullptr;

    for (const LocationConfig& loc : server.getLocations()) {
        const std::string& lp = loc.getPath();
        if (lp.empty()) {
            continue;
        }
        if (lp[0] == '.') {
            if (path.size() >= lp.size() &&
                path.compare(path.size() - lp.size(), lp.size(), lp) == 0) {
                ext = &loc;
            }
            continue;
        }
        if (path.compare(0, lp.size(), lp) != 0) {
            continue;
        }
        if (lp != "/" && path.size() > lp.size() && path[lp.size()] != '/') {
            continue;
        }
        if (lp.size() >= bestLen) {
            bestLen = lp.size();
            best = &loc;
        }
    }
    return ext != nullptr ? ext : best;
}

}  // namespace LocationMatch

void HttpResponse::reset() {
    statusCode_ = 0;
    headers_.clear();
    body_.clear();
    raw_.clear();
    needsCgi_ = false;
}

std::string HttpResponse::statusText(int code) {
    switch (code) {
        case 200:
            return "OK";
        case 204:
            return "No Content";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 400:
            return "Bad Request";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 413:
            return "Payload Too Large";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        default:
            return "Error";
    }
}

bool HttpResponse::buildForPath(const RequestView& request, const ServerConfig& server) {
    const LocationConfig* loc = LocationMatch::match(request.path, server);
    if (loc == nullptr) {
        setError(404, server, nullptr);
        return false;
    }
    build(request, server, *loc);
    return true;
}

void HttpResponse::build(const RequestView& request, const ServerConfig& server,
                         const LocationConfig& location) {
    reset();

    if (request.errorStatus != 0) {
        setError(request.errorStatus, server, &location);
        return;
    }

    const std::uint64_t limit = server.getClientMaxBodySize();
    if (limit != 0 && request.body.size() > static_cast<std::size_t>(limit)) {
        setError(413, server, &location);
        return;
    }

    if (!methodAllowed(request.method, location)) {
        setError(405, server, &location);
        return;
    }

    const auto& redir = location.getRedirection();
    if (redir.first != 0) {
        setRedirect(static_cast<int>(redir.first), redir.second);
        return;
    }

    if (!location.getCgiPass().empty()) {
        needsCgi_ = true;
        return;
    }

    const std::string path = normalizePath(request.path.empty() ? "/" : request.path);
    const std::string fsPath = mapUrlToFs(location, path);
    if (fsPath.empty()) {
        setError(403, server, &location);
        return;
    }

    if (request.method == "DELETE") {
        if (!isFile(fsPath)) {
            setError(404, server, &location);
            return;
        }
        if (::unlink(fsPath.c_str()) != 0) {
            setError(500, server, &location);
            return;
        }
        setEmpty(204);
        return;
    }

    if (request.method == "POST") {
        // Upload handling belongs with Client/Server once wired; not here.
        setError(501, server, &location);
        return;
    }

    // GET
    if (isDir(fsPath)) {
        for (const std::string& index : location.getIndex()) {
            const std::string candidate = joinPath(fsPath, index);
            if (isFile(candidate)) {
                setFile(candidate);
                return;
            }
        }
        if (location.getAutoindex()) {
            setAutoindex(fsPath, path);
            return;
        }
        setError(403, server, &location);
        return;
    }

    if (!isFile(fsPath)) {
        setError(404, server, &location);
        return;
    }
    setFile(fsPath);
}

bool HttpResponse::methodAllowed(const std::string& method, const LocationConfig& loc) const {
    const auto& allowed = loc.getAllowMethods();
    if (allowed.empty()) {
        return method == "GET";
    }
    return std::find(allowed.begin(), allowed.end(), method) != allowed.end();
}

std::string HttpResponse::mapUrlToFs(const LocationConfig& loc, const std::string& urlPath) const {
    const std::string& root = loc.getRoot();
    if (root.empty()) {
        return "";
    }
    const std::string& locPath = loc.getPath();
    std::string relative = urlPath;
    if (locPath != "/" && urlPath.compare(0, locPath.size(), locPath) == 0) {
        relative = urlPath.substr(locPath.size());
        if (relative.empty()) {
            relative = "/";
        }
    }
    if (relative == "/") {
        return root;
    }
    if (!relative.empty() && relative[0] == '/') {
        relative.erase(0, 1);
    }
    return joinPath(root, relative);
}

void HttpResponse::setError(int code, const ServerConfig& server, const LocationConfig* loc) {
    statusCode_ = code;
    body_ = loadErrorPage(code, server, loc);
    headers_.clear();
    headers_["Content-Type"] = "text/html";
    headers_["Content-Length"] = std::to_string(body_.size());
    headers_["Connection"] = "close";
    if (code == 405 && loc != nullptr) {
        std::ostringstream allow;
        const auto& methods = loc->getAllowMethods();
        for (std::size_t i = 0; i < methods.size(); ++i) {
            if (i) {
                allow << ", ";
            }
            allow << methods[i];
        }
        if (methods.empty()) {
            allow << "GET";
        }
        headers_["Allow"] = allow.str();
    }
    finalize();
}

void HttpResponse::setRedirect(int code, const std::string& target) {
    statusCode_ = code;
    body_.clear();
    headers_.clear();
    headers_["Location"] = target;
    headers_["Content-Length"] = "0";
    headers_["Connection"] = "close";
    finalize();
}

void HttpResponse::setFile(const std::string& fsPath) {
    if (!readAll(fsPath, body_)) {
        statusCode_ = 500;
        body_ = defaultErrorBody(500);
        headers_.clear();
        headers_["Content-Type"] = "text/html";
        headers_["Content-Length"] = std::to_string(body_.size());
        headers_["Connection"] = "close";
        finalize();
        return;
    }
    statusCode_ = 200;
    headers_.clear();
    headers_["Content-Type"] = contentType(fsPath);
    headers_["Content-Length"] = std::to_string(body_.size());
    headers_["Connection"] = "close";
    finalize();
}

void HttpResponse::setAutoindex(const std::string& fsDir, const std::string& urlPath) {
    std::string display = urlPath;
    if (display.empty() || display.back() != '/') {
        display.push_back('/');
    }

    std::ostringstream html;
    html << "<html><body><h1>Index of " << display << "</h1><hr><pre>\n";

    DIR* dir = opendir(fsDir.c_str());
    if (dir != nullptr) {
        std::vector<std::string> names;
        while (dirent* entry = readdir(dir)) {
            if (std::string(entry->d_name) == ".") {
                continue;
            }
            names.emplace_back(entry->d_name);
        }
        closedir(dir);
        std::sort(names.begin(), names.end());
        for (const std::string& name : names) {
            html << "<a href=\"" << display << name << "\">" << name << "</a>\n";
        }
    }
    html << "</pre><hr></body></html>\n";

    statusCode_ = 200;
    body_ = html.str();
    headers_.clear();
    headers_["Content-Type"] = "text/html";
    headers_["Content-Length"] = std::to_string(body_.size());
    headers_["Connection"] = "close";
    finalize();
}

void HttpResponse::setEmpty(int code) {
    statusCode_ = code;
    body_.clear();
    headers_.clear();
    headers_["Content-Length"] = "0";
    headers_["Connection"] = "close";
    finalize();
}

void HttpResponse::finalize() {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode_ << " " << statusText(statusCode_) << "\r\n";
    for (const auto& h : headers_) {
        oss << h.first << ": " << h.second << "\r\n";
    }
    oss << "\r\n" << body_;
    raw_ = oss.str();
}

std::string HttpResponse::loadErrorPage(int code, const ServerConfig& server,
                                        const LocationConfig* loc) const {
    const auto& pages = server.getErrorPagePath();
    auto it = pages.find(static_cast<uint16_t>(code));
    if (it == pages.end() && code >= 500 && code <= 504) {
        it = pages.find(500);
    }
    if (it == pages.end()) {
        return defaultErrorBody(code);
    }

    std::vector<std::string> candidates;
    const std::string& configured = it->second;
    if (!configured.empty() && configured[0] == '/') {
        candidates.push_back(configured);
    }
    if (loc != nullptr && !loc->getRoot().empty()) {
        candidates.push_back(joinPath(loc->getRoot(), configured));
    }
    for (const LocationConfig& l : server.getLocations()) {
        if (l.getPath() == "/" && !l.getRoot().empty()) {
            candidates.push_back(joinPath(l.getRoot(), configured));
            break;
        }
    }

    for (const std::string& path : candidates) {
        std::string content;
        if (isFile(path) && readAll(path, content)) {
            return content;
        }
    }
    return defaultErrorBody(code);
}

std::string HttpResponse::defaultErrorBody(int code) {
    return "<html><body><h1>" + std::to_string(code) + " " + statusText(code) +
           "</h1></body></html>\n";
}

std::string HttpResponse::contentType(const std::string& path) {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return "application/octet-stream";
    }
    const std::string ext = toLower(path.substr(dot + 1));
    if (ext == "html" || ext == "htm") {
        return "text/html";
    }
    if (ext == "css") {
        return "text/css";
    }
    if (ext == "js") {
        return "application/javascript";
    }
    if (ext == "png") {
        return "image/png";
    }
    if (ext == "jpg" || ext == "jpeg") {
        return "image/jpeg";
    }
    if (ext == "txt") {
        return "text/plain";
    }
    return "application/octet-stream";
}

std::string HttpResponse::joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) {
        return b;
    }
    if (b.empty()) {
        return a;
    }
    std::string right = b;
    if (right[0] == '/') {
        right.erase(0, 1);
    }
    if (a.back() == '/') {
        return a + right;
    }
    return a + "/" + right;
}

bool HttpResponse::isDir(const std::string& path) {
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool HttpResponse::isFile(const std::string& path) {
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool HttpResponse::readAll(const std::string& path, std::string& out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    out = oss.str();
    return true;
}
