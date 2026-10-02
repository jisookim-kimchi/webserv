#include "../includes/CgiHandler.hpp"
#include "test_utils.hpp"

#include <unistd.h>

#include <cstdlib>
#include <ctime>
#include <map>
#include <string>

namespace {

std::string python3() {
    const char* fromEnv = std::getenv("WEBSERV_PYTHON");
    if (fromEnv && *fromEnv)
        return fromEnv;
    if (access("/usr/bin/python3", X_OK) == 0)
        return "/usr/bin/python3";
    if (access("/opt/homebrew/bin/python3", X_OK) == 0)
        return "/opt/homebrew/bin/python3";
    return "python3";
}

std::string headerValue(const std::map<std::string, std::string>& headers, const std::string& key) {
    const auto it = headers.find(key);
    expectTrue(it != headers.end(), "missing header: " + key);
    return it->second;
}

CgiHandler::Request baseRequest(const std::string& script) {
    const std::string root = projectRoot();
    CgiHandler::Request request;
    request.interpreterPath = python3();
    request.scriptPath = root + "/www/cgi-bin/" + script;
    request.documentRoot = root + "/www";
    request.requestMethod = "GET";
    request.scriptName = "/cgi-bin/" + script;
    request.requestUri = request.scriptName;
    request.serverName = "127.0.0.1";
    request.serverPort = "8080";
    request.remoteAddr = "127.0.0.1";
    request.headers["Host"] = "127.0.0.1:8080";
    return request;
}

void echo_post_body_and_query() {
    auto request = baseRequest("echo.py");
    request.requestMethod = "POST";
    request.queryString = "name=webserv";
    request.requestUri = "/cgi-bin/echo.py?name=webserv";
    request.contentType = "text/plain";
    request.requestBody = "hello from cgi";
    request.headers["User-Agent"] = "webserv-tests";

    const CgiHandler::Result result = CgiHandler::execute(request);
    EXPECT_EQ(result.statusCode, 200);
    EXPECT_EQ(headerValue(result.headers, "Content-Type"), std::string("text/plain"));
    EXPECT_CONTAINS(result.body, "CGI_OK");
    EXPECT_CONTAINS(result.body, "REQUEST_METHOD=POST");
    EXPECT_CONTAINS(result.body, "QUERY_STRING=name=webserv");
    EXPECT_CONTAINS(result.body, "CONTENT_LENGTH=14");
    EXPECT_CONTAINS(result.body, "BODY=hello from cgi");
}

void hello_get_html() {
    auto request = baseRequest("hello.py");
    request.queryString = "name=42";
    request.requestUri = "/cgi-bin/hello.py?name=42";

    const CgiHandler::Result result = CgiHandler::execute(request);
    EXPECT_EQ(result.statusCode, 200);
    EXPECT_CONTAINS(headerValue(result.headers, "Content-Type"), "text/html");
    EXPECT_CONTAINS(result.body, "Hello, 42!");
}

void redirect_status_header() {
    auto request = baseRequest("redirect.py");
    const CgiHandler::Result result = CgiHandler::execute(request);
    EXPECT_EQ(result.statusCode, 302);
    EXPECT_EQ(headerValue(result.headers, "Location"), std::string("/elsewhere"));
    EXPECT_CONTAINS(result.body, "redirect body");
}

void env_http_headers() {
    auto request = baseRequest("env.py");
    request.headers["User-Agent"] = "cgi-suite";
    const CgiHandler::Result result = CgiHandler::execute(request);
    EXPECT_EQ(result.statusCode, 200);
    EXPECT_CONTAINS(result.body, "GATEWAY_INTERFACE=CGI/1.1");
    EXPECT_CONTAINS(result.body, "REQUEST_METHOD=GET");
    EXPECT_CONTAINS(result.body, "HTTP_HOST=127.0.0.1:8080");
    EXPECT_CONTAINS(result.body, "HTTP_USER_AGENT=cgi-suite");
}

void relative_working_directory() {
    auto request = baseRequest("relative.py");
    const CgiHandler::Result result = CgiHandler::execute(request);
    EXPECT_EQ(result.statusCode, 200);
    EXPECT_CONTAINS(result.body, "RELATIVE_OK=cgi-bin-ok");
}

void timeout_kills_slow_script() {
    auto request = baseRequest("slow.py");
    const time_t started = time(nullptr);
    bool timedOut = false;
    try {
        CgiHandler::execute(request, 1000);
    } catch (const std::exception& ex) {
        timedOut = std::string(ex.what()).find("timed out") != std::string::npos;
        EXPECT_TRUE(timedOut);
    }
    EXPECT_TRUE(timedOut);
    EXPECT_TRUE(time(nullptr) - started <= 3);
}

void execute_session_api() {
    auto request = baseRequest("redirect.py");
    const CgiHandler::Result result = CgiHandler::execute(request, 3000);
    EXPECT_EQ(result.statusCode, 302);
    EXPECT_EQ(headerValue(result.headers, "Location"), std::string("/elsewhere"));
}

void missing_script_throws() {
    auto request = baseRequest("does-not-exist.py");
    bool threw = false;
    try {
        CgiHandler::execute(request);
    } catch (const std::exception&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

}  // namespace

int main() {
    std::cout << "== cgi ==\n";
    int failed = 0;
    failed += runTest("echo_post_body_and_query", echo_post_body_and_query);
    failed += runTest("hello_get_html", hello_get_html);
    failed += runTest("redirect_status_header", redirect_status_header);
    failed += runTest("env_http_headers", env_http_headers);
    failed += runTest("relative_working_directory", relative_working_directory);
    failed += runTest("timeout_kills_slow_script", timeout_kills_slow_script);
    failed += runTest("execute_session_api", execute_session_api);
    failed += runTest("missing_script_throws", missing_script_throws);
    std::cout << "-- cgi: " << (8 - failed) << "/8 passed\n";
    return failed == 0 ? 0 : 1;
}
