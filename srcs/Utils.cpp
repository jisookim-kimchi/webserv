#include "../includes/Utils.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace Utils {

std::string toLower(const std::string& value) {
    std::string result = value;
    for (char& c : result)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

std::string trimLeft(const std::string& value) {
    size_t i = 0;
    while (i < value.size() && (value[i] == ' ' || value[i] == '\t'))
        ++i;
    return value.substr(i);
}

std::string joinPath(const std::string& left, const std::string& right) {
    if (left.empty())
        return right;
    if (right.empty())
        return left;
    if (left.back() == '/' && right.front() == '/')
        return left + right.substr(1);
    if (left.back() != '/' && right.front() != '/')
        return left + "/" + right;
    return left + right;
}

std::string dirName(const std::string& path) {
    if (path.empty())
        return ".";
    const size_t pos = path.find_last_of('/');
    if (pos == std::string::npos)
        return ".";
    if (pos == 0)
        return "/";
    return path.substr(0, pos);
}

std::string headerToCgiEnvKey(const std::string& headerName) {
    std::string key = "HTTP_";
    for (unsigned char c : headerName) {
        if (c == '-')
            key.push_back('_');
        else
            key.push_back(static_cast<char>(std::toupper(c)));
    }
    return key;
}

void setNonBlocking(int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        throw std::runtime_error(std::string("fcntl(O_NONBLOCK) failed: ") + std::strerror(errno));
}

bool isReadableFile(const std::string& path) {
    return !path.empty() && access(path.c_str(), R_OK) == 0;
}

bool isExecutableFile(const std::string& path) {
    return !path.empty() && access(path.c_str(), X_OK) == 0;
}

UniqueFd::UniqueFd(int fd) noexcept : fd_(fd) {
}

UniqueFd::~UniqueFd() {
    reset();
}

UniqueFd::UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

UniqueFd& UniqueFd::operator=(UniqueFd&& other) noexcept {
    if (this != &other) {
        reset();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

int UniqueFd::get() const noexcept {
    return fd_;
}

int UniqueFd::release() noexcept {
    const int fd = fd_;
    fd_ = -1;
    return fd;
}

void UniqueFd::reset(int fd) noexcept {
    if (fd_ >= 0)
        close(fd_);
    fd_ = fd;
}

UniqueFd::operator bool() const noexcept {
    return fd_ >= 0;
}

}  // namespace Utils
