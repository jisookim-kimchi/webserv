#include "../includes/HTTP_Request.hpp"
#include "test_utils.hpp"
#include <iostream>
#include <string>

namespace {

void test_get() {
    std::string raw = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getMethodString(), "GET");
    EXPECT_EQ(req.getPath(), "/index.html");
    EXPECT_EQ(req.getQueryString(), "");
    EXPECT_EQ(req.getHeader("host"), "localhost:8080");
}

void test_query() {
    std::string raw = 
        "GET /jisookim?test=4242%heilbronn HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getPath(), "/jisookim");
    EXPECT_EQ(req.getQueryString(), "test=4242%heilbronn");
}

void test_post() {
    std::string raw = 
        "POST /api/login HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "hello world";

    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getMethodString(), "POST");
    EXPECT_EQ(req.getBody(), "hello world");
    EXPECT_EQ(req.getHeader("content-type"), "text/plain");
    EXPECT_EQ(req.getBody().size(), 11UL);
}

void test_headers() {
    std::string raw = 
        "GET /test HTTP/1.1\r\n"
        "HOST:\tlocalhost:8080\r\n"
        "User-Agent:    curl/7.68.0\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getHeader("host"), "localhost:8080");
    EXPECT_EQ(req.getHeader("user-agent"), "curl/7.68.0");
}

void test_no_host() {
    std::string raw = 
        "GET /index.html HTTP/1.1\r\n"
        "User-Agent: curl/7.68.0\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

void test_dup_host() {
    std::string raw = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: first.com\r\n"
        "Host: second.com\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

void test_bad_method() {
    std::string raw = 
        "FOOBAR /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

void test_bad_version() {
    std::string raw = 
        "GET /index.html HTTP/1.0\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

// send fragmented body
void test_chunked() {
    std::string raw = 
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "6\r\n"
        " World\r\n"
        "0\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getBody(), "Hello World"); 
}

//test url decode 
void test_url_decode() {
    std::string raw = 
        "GET /my%20folder/file%2Btest.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getPath(), "/my folder/file+test.html"); 
}

//test length mismatch
void test_len_mismatch() {
    std::string raw = 
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 100\r\n"
        "\r\n"
        "hello";

    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

//test invalid content length
void test_bad_len() {
    std::string raw = 
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: abc12\r\n"
        "\r\n"
        "hello";

    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

//test both length and chunked
void test_len_and_chunked() {
    std::string raw = 
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 5\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\nhello\r\n0\r\n\r\n";

    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

} // namespace

int main() {
    int failed = 0;

    failed += runTest("test_get", &test_get);
    failed += runTest("test_query", &test_query);
    failed += runTest("test_post", &test_post);
    failed += runTest("test_headers", &test_headers);
    failed += runTest("test_no_host", &test_no_host);
    failed += runTest("test_dup_host", &test_dup_host);
    failed += runTest("test_bad_method", &test_bad_method);
    failed += runTest("test_bad_version", &test_bad_version);
    std::cout << "\n ----------------------------------------\n";
    failed += runTest("test_chunked", &test_chunked);
    failed += runTest("test_url_decode", &test_url_decode);
    failed += runTest("test_len_mismatch", &test_len_mismatch);
    failed += runTest("test_bad_len", &test_bad_len);
    failed += runTest("test_len_and_chunked", &test_len_and_chunked);

    if (failed == 0) {
        std::cout << "\n All HTTP_Request tests passed!\n";
    } else {
        std::cout << "\n" << failed << " tests failed\n";
    }
    return failed;
}
