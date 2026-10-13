#include "../includes/Utils.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace Utils {

std::string toLower(const std::string& value) {
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

std::string trimLeft(const std::string& value) {
    const size_t i = value.find_first_not_of(" \t");
    return i == std::string::npos ? std::string() : value.substr(i);
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

bool hexCharToInt(char c, size_t& digit) {
    if (c >= '0' && c <= '9') {
        digit = static_cast<size_t>(c - '0');
        return true;
    }
    const char lower = static_cast<char>(c | 0x20);
    if (lower >= 'a' && lower <= 'f') {
        digit = static_cast<size_t>(lower - 'a' + 10);
        return true;
    }
    return false;
}

bool parseHexSize(const std::string& s, size_t pos, size_t len, size_t& out) {
    if (len == 0 || pos + len > s.size())
        return false;
    unsigned long long value = 0;
    const char* first = s.data() + pos;
    const char* last = first + len;
    const auto result = std::from_chars(first, last, value, 16);
    if (result.ec != std::errc() || result.ptr != last)
        return false;
    if (value > static_cast<unsigned long long>(std::numeric_limits<size_t>::max()))
        return false;
    out = static_cast<size_t>(value);
    return true;
}

bool parseHexSize(const std::string& s, size_t& out) {
    return parseHexSize(s, 0, s.size(), out);
}

bool parseSizeDecimal(const std::string& s, size_t& out) {
    if (s.empty())
        return false;
    unsigned long long value = 0;
    const char* first = s.data();
    const char* last = s.data() + s.size();
    const auto result = std::from_chars(first, last, value, 10);
    if (result.ec != std::errc() || result.ptr != last)
        return false;
    if (value > static_cast<unsigned long long>(std::numeric_limits<size_t>::max()))
        return false;
    out = static_cast<size_t>(value);
    return true;
}

void setNonBlocking(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
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
