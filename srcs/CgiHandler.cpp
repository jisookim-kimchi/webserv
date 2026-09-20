#include "../includes/CgiHandler.hpp"

#include "../includes/Utils.hpp"

#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <iostream> // for DEBUG

namespace {

std::vector<std::string> buildEnvironment(const CgiHandler::Request& request) {
    std::vector<std::string> env;

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("SERVER_SOFTWARE=webserv");
    env.push_back("REQUEST_METHOD=" + request.requestMethod);
    env.push_back("REQUEST_URI=" + request.requestUri);
    env.push_back("QUERY_STRING=" + request.queryString);
    env.push_back("SCRIPT_NAME=" + request.scriptName);
    env.push_back("SCRIPT_FILENAME=" + request.scriptPath);
    env.push_back("DOCUMENT_ROOT=" + request.documentRoot);
    env.push_back("PATH_INFO=" + request.pathInfo);
    env.push_back("PATH_TRANSLATED=" + request.pathTranslated);
    env.push_back("SERVER_NAME=" + request.serverName);
    env.push_back("SERVER_PORT=" + request.serverPort);
    env.push_back("REMOTE_ADDR=" + (request.remoteAddr.empty() ? "127.0.0.1" : request.remoteAddr));
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("CONTENT_LENGTH=" + std::to_string(request.requestBody.size()));

    if (!request.contentType.empty())
        env.push_back("CONTENT_TYPE=" + request.contentType);

    for (const auto& [name, value] : request.headers) {
        const std::string lower = Utils::toLower(name);
        if (lower == "content-type" || lower == "content-length")
            continue;
        env.push_back(Utils::headerToCgiEnvKey(name) + "=" + value);
    }

    for (const auto& [key, value] : request.extraEnv)
        env.push_back(key + "=" + value);

    return env;
}

std::vector<char*> toCStringArray(std::vector<std::string>& values) {
    std::vector<char*> result;
    result.reserve(values.size() + 1);
    for (auto& value : values)
        result.push_back(value.data());
    result.push_back(nullptr);
    return result;
}

std::string stripCr(const std::string& line) {
    if (!line.empty() && line.back() == '\r')
        return line.substr(0, line.size() - 1);
    return line;
}

// Drop inherited listen/client sockets so the CGI child cannot hold them open.
void closeInheritedFds() {
    const int maxFd = 1024;
    for (int fd = 3; fd < maxFd; ++fd)
        close(fd);
}

}  // namespace

CgiHandler::~CgiHandler() {
    reset();
}

void CgiHandler::reset() {
    if (pid_ > 0) {
        kill(pid_, SIGKILL);
        int status = 0;
        while (waitpid(pid_, &status, 0) < 0) {
            if (errno != EINTR)
                break;
        }
        pid_ = -1;
    }
    closeStdin();
    closeStdout();
    body_.clear();
    bodyOffset_ = 0;
    rawOutput_.clear();
    result_ = Result{};
    errorMessage_.clear();
    deadline_ = 0;
    state_ = State::Idle;
}

void CgiHandler::cancel() {
    if (state_ != State::Running)
        return;
    if (pid_ > 0)
        kill(pid_, SIGKILL);
    closeStdin();
    closeStdout();
    reapChild();
    state_ = State::TimedOut;
    errorMessage_ = "CGI execution cancelled";
}

void CgiHandler::fail(const std::string& message) {
    if (pid_ > 0)
        kill(pid_, SIGKILL);
    closeStdin();
    closeStdout();
    if (pid_ > 0)
        reapChild();
    state_ = State::Error;
    errorMessage_ = message;
}

void CgiHandler::closeStdin() {
    if (stdinFd_ >= 0) {
        close(stdinFd_);
        stdinFd_ = -1;
    }
}

void CgiHandler::closeStdout() {
    if (stdoutFd_ >= 0) {
        close(stdoutFd_);
        stdoutFd_ = -1;
    }
}

void CgiHandler::reapChild() {
    if (pid_ <= 0)
        return;

    int status = 0;
    pid_t waited = waitpid(pid_, &status, WNOHANG);
    if (waited == 0) {
        // Still running after pipes closed: do not block the event loop forever.
        kill(pid_, SIGKILL);
        while ((waited = waitpid(pid_, &status, 0)) < 0) {
            if (errno != EINTR)
                break;
        }
    } else if (waited < 0) {
        while ((waited = waitpid(pid_, &status, 0)) < 0) {
            if (errno != EINTR)
                break;
        }
    }

    if (waited > 0)
        result_.exitStatus = status;
    pid_ = -1;
}

void CgiHandler::markDone() {
    closeStdin();
    closeStdout();
    reapChild();

    if (WIFEXITED(result_.exitStatus) && WEXITSTATUS(result_.exitStatus) == 127) {
        state_ = State::Error;
        errorMessage_ = "CGI execve failed";
        return;
    }

    result_ = parseOutput(rawOutput_, result_.exitStatus);
    state_ = State::Done;
}

CgiHandler::State CgiHandler::state() const {
    return state_;
}

bool CgiHandler::isFinished() const {
    return state_ == State::Done || state_ == State::Error || state_ == State::TimedOut;
}

bool CgiHandler::isRunning() const {
    return state_ == State::Running;
}

const CgiHandler::Result& CgiHandler::result() const {
    return result_;
}

const std::string& CgiHandler::errorMessage() const {
    return errorMessage_;
}

int CgiHandler::stdinFd() const {
    return stdinFd_;
}

int CgiHandler::stdoutFd() const {
    return stdoutFd_;
}

short CgiHandler::stdinEvents() const {
    return (state_ == State::Running && stdinFd_ >= 0) ? static_cast<short>(POLLOUT) : 0;
}

short CgiHandler::stdoutEvents() const {
    return (state_ == State::Running && stdoutFd_ >= 0)
               ? static_cast<short>(POLLIN | POLLHUP | POLLERR)
               : 0;
}

void CgiHandler::validate(const Request& request) const {
    if (request.scriptPath.empty())
        throw std::runtime_error("CGI script path is empty");
    if (!Utils::isReadableFile(request.scriptPath))
        throw std::runtime_error("CGI script is not readable: " + request.scriptPath);

    if (!request.interpreterPath.empty()) {
        if (!Utils::isExecutableFile(request.interpreterPath) &&
            !Utils::isReadableFile(request.interpreterPath))
            throw std::runtime_error("CGI interpreter is not usable: " + request.interpreterPath);
    } else if (!Utils::isExecutableFile(request.scriptPath)) {
        throw std::runtime_error("CGI script is not executable: " + request.scriptPath);
    }
}

void CgiHandler::spawnChild(const Request& request, int stdinRead, int stdinWrite, int stdoutRead,
                            int stdoutWrite) {
    if (dup2(stdinRead, STDIN_FILENO) < 0)
        _exit(127);
    if (dup2(stdoutWrite, STDOUT_FILENO) < 0)
        _exit(127);

    close(stdinRead);
    close(stdinWrite);
    close(stdoutRead);
    close(stdoutWrite);
    closeInheritedFds();
    const std::string cwd = request.workingDirectory.empty() ? Utils::dirName(request.scriptPath)
                                                             : request.workingDirectory;
    if (!cwd.empty())
        chdir(cwd.c_str());
    auto envStrings = buildEnvironment(request);
    auto envp = toCStringArray(envStrings);

    std::vector<std::string> argStrings;
    if (!request.interpreterPath.empty()) {
        argStrings.push_back(request.interpreterPath);
        argStrings.push_back(request.scriptPath);
        auto argv = toCStringArray(argStrings);
        execve(request.interpreterPath.c_str(), argv.data(), envp.data());
    } else {
        argStrings.push_back(request.scriptPath);
        auto argv = toCStringArray(argStrings);
        execve(request.scriptPath.c_str(), argv.data(), envp.data());
    }
    perror("execve failed");
    _exit(127);
}

void CgiHandler::launch(const Request& request, int timeoutMs) {
    reset();
    validate(request);

    int stdinPipe[2];
    int stdoutPipe[2];
    if (pipe(stdinPipe) < 0)
        throw std::runtime_error(std::string("pipe failed: ") + std::strerror(errno));
    if (pipe(stdoutPipe) < 0) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        throw std::runtime_error(std::string("pipe failed: ") + std::strerror(errno));
    }

    Utils::UniqueFd stdinRead(stdinPipe[0]);
    Utils::UniqueFd stdinWrite(stdinPipe[1]);
    Utils::UniqueFd stdoutRead(stdoutPipe[0]);
    Utils::UniqueFd stdoutWrite(stdoutPipe[1]);

    const pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error(std::string("fork failed: ") + std::strerror(errno));

    if (pid == 0) {
        spawnChild(request, stdinRead.get(), stdinWrite.get(), stdoutRead.get(), stdoutWrite.get());
    }

    // Parent keeps write-end of stdin pipe and read-end of stdout pipe.
    stdinRead.reset();
    stdoutWrite.reset();

    Utils::setNonBlocking(stdinWrite.get());
    Utils::setNonBlocking(stdoutRead.get());

    pid_ = pid;
    body_ = request.requestBody;
    bodyOffset_ = 0;
    rawOutput_.clear();
    result_ = Result{};
    errorMessage_.clear();
    deadline_ = (timeoutMs > 0) ? time(nullptr) + (timeoutMs + 999) / 1000 : 0;
    state_ = State::Running;

    stdinFd_ = stdinWrite.release();
    stdoutFd_ = stdoutRead.release();

    // Empty body: close stdin immediately so CGI sees EOF.
    if (body_.empty())
        closeStdin();
}

void CgiHandler::onStdinReady() {
    if (state_ != State::Running || stdinFd_ < 0)
        return;

    const char* data = body_.data() + bodyOffset_;
    const size_t remaining = body_.size() - bodyOffset_;
    const ssize_t written = write(stdinFd_, data, remaining);

    // rule.md: do not inspect errno after write. Partial writes wait for next POLLOUT.
    // Negative: script closed stdin early — stop feeding and keep reading stdout.
    if (written < 0) {
        closeStdin();
        if (stdoutFd_ < 0)
            markDone();
        return;
    }

    bodyOffset_ += static_cast<size_t>(written);
    if (bodyOffset_ >= body_.size()) {
        closeStdin();  // EOF for CGI (rule 4.4.2)
        if (stdoutFd_ < 0)
            markDone();
    }
}

void CgiHandler::onStdoutReady() {
    if (state_ != State::Running || stdoutFd_ < 0)
        return;

    char buffer[4096];
    const ssize_t nread = read(stdoutFd_, buffer, sizeof(buffer));

    // rule.md: do not inspect errno after read.
    //   > 0  keep collecting
    //   = 0  EOF (rule 4.4.3: read until EOF)
    //   < 0  pipe error
    if (nread > 0) {
        rawOutput_.append(buffer, static_cast<size_t>(nread));
        return;
    }
    if (nread == 0) {
        closeStdout();
        if (stdinFd_ < 0)
            markDone();
        return;
    }

    fail("CGI stdout read failed");
}

void CgiHandler::checkTimeout() {
    if (state_ != State::Running || deadline_ == 0)
        return;
    if (time(nullptr) < deadline_)
        return;

    if (pid_ > 0)
        kill(pid_, SIGKILL);
    closeStdin();
    closeStdout();
    reapChild();
    state_ = State::TimedOut;
    errorMessage_ = "CGI execution timed out";
}

CgiHandler::Result CgiHandler::parseOutput(const std::string& raw, int waitStatus) const {
    Result result;
    result.exitStatus = waitStatus;
    result.statusCode = 200;
    result.rawOutput = raw;

    size_t headerEnd = raw.find("\r\n\r\n");
    size_t delimiter = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = raw.find("\n\n");
        delimiter = 2;
    }

    if (headerEnd == std::string::npos) {
        result.body = raw;
        return result;
    }

    result.headerBlock = raw.substr(0, headerEnd);
    result.body = raw.substr(headerEnd + delimiter);

    std::istringstream stream(result.headerBlock);
    std::string line;
    while (std::getline(stream, line)) {
        line = stripCr(line);
        if (line.empty())
            continue;

        const size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        const std::string key = line.substr(0, colon);
        const std::string value = Utils::trimLeft(line.substr(colon + 1));
        result.headers[key] = value;

        if (Utils::toLower(key) == "status") {
            std::istringstream statusStream(value);
            statusStream >> result.statusCode;
            if (result.statusCode <= 0)
                result.statusCode = 200;
        }
    }

    return result;
}

CgiHandler::Result CgiHandler::execute(const Request& request, int timeoutMs) {
    CgiHandler cgi;
    cgi.launch(request, timeoutMs);

    while (cgi.isRunning()) {
        cgi.checkTimeout();
        if (!cgi.isRunning())
            break;

        pollfd fds[2];
        nfds_t nfds = 0;
        int stdinIndex = -1;
        int stdoutIndex = -1;

        if (cgi.stdinEvents() != 0) {
            stdinIndex = static_cast<int>(nfds);
            fds[nfds].fd = cgi.stdinFd();
            fds[nfds].events = cgi.stdinEvents();
            fds[nfds].revents = 0;
            ++nfds;
        }
        if (cgi.stdoutEvents() != 0) {
            stdoutIndex = static_cast<int>(nfds);
            fds[nfds].fd = cgi.stdoutFd();
            fds[nfds].events = cgi.stdoutEvents();
            fds[nfds].revents = 0;
            ++nfds;
        }

        if (nfds == 0)
            break;

        const int ready = poll(fds, nfds, 200);
        if (ready < 0) {
            if (errno == EINTR)
                continue;
            cgi.fail(std::string("poll failed: ") + std::strerror(errno));
            break;
        }
        if (ready == 0)
            continue;

        if (stdinIndex >= 0 && (fds[stdinIndex].revents & (POLLOUT | POLLERR | POLLHUP | POLLNVAL)))
            cgi.onStdinReady();
        if (cgi.isRunning() && stdoutIndex >= 0 &&
            (fds[stdoutIndex].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)))
            cgi.onStdoutReady();
    }

    if (cgi.state() == State::Done)
        return cgi.result();
    if (cgi.state() == State::TimedOut)
        throw std::runtime_error(cgi.errorMessage());
    if (cgi.state() == State::Error)
        throw std::runtime_error(cgi.errorMessage());
    throw std::runtime_error("CGI ended in unexpected state");
}
