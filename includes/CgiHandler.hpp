#pragma once

#include <ctime>
#include <map>
#include <string>
#include <sys/types.h>

// CGI execution module (Overview) constrained by rule.md:
//   - fork() only for CGI
//   - pipes are non-blocking and must be registered in Server's single poll()
//   - never read/write a pipe unless poll marked it ready
//   - never branch on errno after read/write
//
// Ownership model:
//   Client holds one CgiHandler while a CGI request is in flight.
//   Server adds stdinFd()/stdoutFd() into the shared poll set.
//   HttpRequest must already have un-chunked the body before launch().
class CgiHandler {
   public:
    enum class State { Idle, Running, Done, Error, TimedOut };

    struct Request {
        std::string interpreterPath;   // LocationConfig::cgi_pass
        std::string scriptPath;        // filesystem path of the script
        std::string workingDirectory;  // empty => dirname(scriptPath)
        std::string requestMethod;
        std::string requestUri;
        std::string queryString;
        std::string contentType;
        std::string requestBody;  // already decoded / un-chunked
        std::string scriptName;
        std::string pathInfo;
        std::string pathTranslated;
        std::string documentRoot;
        std::string serverName;
        std::string serverPort;
        std::string remoteAddr;
        // Original HTTP headers; converted to HTTP_* CGI variables.
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> extraEnv;
    };

    // Parsed CGI output for HttpResponse to turn into an HTTP message.
    struct Result {
        int exitStatus = 0;
        int statusCode = 200;
        std::string rawOutput;
        std::string headerBlock;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    CgiHandler() = default;
    ~CgiHandler();

    CgiHandler(const CgiHandler&) = delete;
    CgiHandler& operator=(const CgiHandler&) = delete;

    // fork + pipe setup. Body is fed later through onStdinReady().
    void launch(const Request& request, int timeoutMs = 5000);

    // --- poll registration (Server) ---
    int stdinFd() const;
    int stdoutFd() const;
    short stdinEvents() const;
    short stdoutEvents() const;

    // --- poll dispatch (call only when the matching fd is ready) ---
    void onStdinReady();
    void onStdoutReady();
    void checkTimeout();

    State state() const;
    bool isFinished() const;
    bool isRunning() const;

    const Result& result() const;
    const std::string& errorMessage() const;

    void cancel();
    void reset();

    // Test helper: drives this handler with a private poll loop.
    // Production Server must use the single shared poll instead.
    static Result execute(const Request& request, int timeoutMs = 5000);

   private:
    void fail(const std::string& message);
    void markDone();
    void closeStdin();
    void closeStdout();
    void reapChild();
    void validate(const Request& request) const;
    void spawnChild(const Request& request, int stdinRead, int stdinWrite, int stdoutRead,
                    int stdoutWrite);
    Result parseOutput(const std::string& raw, int waitStatus) const;

    State state_ = State::Idle;
    pid_t pid_ = -1;
    int stdinFd_ = -1;   // parent write end
    int stdoutFd_ = -1;  // parent read end
    std::string body_;
    size_t bodyOffset_ = 0;
    std::string rawOutput_;
    Result result_;
    std::string errorMessage_;
    time_t deadline_ = 0;
};
