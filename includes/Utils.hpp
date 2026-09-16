#pragma once

#include <string>

namespace Utils {

std::string toLower(const std::string& value);
std::string trimLeft(const std::string& value);
std::string joinPath(const std::string& left, const std::string& right);
std::string dirName(const std::string& path);
std::string headerToCgiEnvKey(const std::string& headerName);

void setNonBlocking(int fd);
bool isReadableFile(const std::string& path);
bool isExecutableFile(const std::string& path);

// RAII file descriptor. Non-copyable, movable.
class UniqueFd {
   public:
    explicit UniqueFd(int fd = -1) noexcept;
    ~UniqueFd();

    UniqueFd(UniqueFd&& other) noexcept;
    UniqueFd& operator=(UniqueFd&& other) noexcept;

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    int get() const noexcept;
    int release() noexcept;
    void reset(int fd = -1) noexcept;
    explicit operator bool() const noexcept;

   private:
    int fd_;
};

}  // namespace Utils
