#include "../includes/HTTP_Request.hpp"
#include "test_utils.hpp"
#include <iostream>
#include <sstream>
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
void test_complex_chunked()
{
    std::string raw = 
        "POST /upload/stream HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Type: application/octet-stream\r\n"
        "\r\n"
        "1a\r\n"
        "abcdefghijklmnopqrstuvwxyz\r\n"
        "F\r\n"
        "0123456789ABCDE\r\n"
        "5\r\n" 
        "FINAL\r\n"
        "0\r\n"
        "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getBody(), "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFINAL");
    EXPECT_EQ(req.getBody().size(), 46UL);
}

void test_complex_uri_and_query() {
    std::string raw = 
        "GET /api/v1/user%2Fprofile/special%21%40%23?name=jisoo%20kim&age=42&sort=asc%26desc HTTP/1.1\r\n"
        "Host: 42heilbronn.de:443\r\n"
        "Accept: text/html,application/xhtml+xml\r\n"
        "\r\n";

    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getPath(), "/api/v1/user/profile/special!@#");
    EXPECT_EQ(req.getQueryString(), "name=jisoo%20kim&age=42&sort=asc%26desc");
    EXPECT_EQ(req.getHeader("host"), "42heilbronn.de:443");
    EXPECT_EQ(req.getHeader("accept"), "text/html,application/xhtml+xml");
}

void test_large_body() {
    const std::string payload(256 * 1024, 'A');
    std::ostringstream oss;
    oss << "POST /upload HTTP/1.1\r\n"
        << "Host: localhost\r\n"
        << "Content-Length: " << payload.size() << "\r\n"
        << "\r\n"
        << payload;
    HTTP_Request req;
    EXPECT_TRUE(req.parse(oss.str()));
    EXPECT_EQ(req.getBody().size(), payload.size());
    EXPECT_EQ(req.getBody()[0], 'A');
    EXPECT_EQ(req.getBody()[payload.size() - 1], 'A');
}

void test_many_headers() {
    std::ostringstream oss;
    oss << "GET /h HTTP/1.1\r\nHost: localhost\r\n";
    for (int i = 0; i < 200; ++i)
        oss << "X-Custom-" << i << ": value-" << i << "\r\n";
    oss << "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(req.parse(oss.str()));
    EXPECT_EQ(req.getHeader("x-custom-0"), "value-0");
    EXPECT_EQ(req.getHeader("x-custom-199"), "value-199");
    EXPECT_EQ(req.getHeaders().size(), 201UL);  // host + 200
}

void test_empty_header_value() {
    std::string raw =
        "GET /x HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-Empty:\r\n"
        "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getHeader("x-empty"), "");
}

void test_crlf_only_after_headers_zero_cl() {
    std::string raw =
        "POST /z HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getBody(), "");
}

void test_bad_percent_encoding() {
    std::string raw =
        "GET /bad%zz HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

void test_incomplete_percent() {
    std::string raw =
        "GET /bad%2 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

void test_path_must_start_with_slash() {
    std::string raw =
        "GET index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

void test_chunked_incomplete() {
    std::string raw =
        "POST /c HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "Hel";
    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

void test_chunked_bad_size() {
    std::string raw =
        "POST /c HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "ZZ\r\n"
        "nope\r\n"
        "0\r\n"
        "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(!req.parse(raw));
}

void test_tab_folded_spacing_in_headers() {
    std::string raw =
        "GET /t HTTP/1.1\r\n"
        "Host:\t\tlocalhost\t\r\n"
        "X-Data:    spaced   value\r\n"
        "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getHeader("host"), "localhost\t");
    EXPECT_EQ(req.getHeader("x-data"), "spaced   value");
}

void test_long_query_string() {
    std::string q(8000, 'q');
    std::string raw =
        "GET /search?" + q + " HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    HTTP_Request req;
    EXPECT_TRUE(req.parse(raw));
    EXPECT_EQ(req.getQueryString().size(), 8000UL);
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
    failed += runTest("test_chunked", &test_chunked);
    failed += runTest("test_url_decode", &test_url_decode);
    failed += runTest("test_len_mismatch", &test_len_mismatch);
    failed += runTest("test_bad_len", &test_bad_len);
    failed += runTest("test_len_and_chunked", &test_len_and_chunked);
    failed += runTest("test_complex_chunked", &test_complex_chunked);
    failed += runTest("test_complex_uri_and_query", &test_complex_uri_and_query);
    failed += runTest("test_large_body", &test_large_body);
    failed += runTest("test_many_headers", &test_many_headers);
    failed += runTest("test_empty_header_value", &test_empty_header_value);
    failed += runTest("test_crlf_only_after_headers_zero_cl", &test_crlf_only_after_headers_zero_cl);
    failed += runTest("test_bad_percent_encoding", &test_bad_percent_encoding);
    failed += runTest("test_incomplete_percent", &test_incomplete_percent);
    failed += runTest("test_path_must_start_with_slash", &test_path_must_start_with_slash);
    failed += runTest("test_chunked_incomplete", &test_chunked_incomplete);
    failed += runTest("test_chunked_bad_size", &test_chunked_bad_size);
    failed += runTest("test_tab_folded_spacing_in_headers", &test_tab_folded_spacing_in_headers);
    failed += runTest("test_long_query_string", &test_long_query_string);
    if (failed == 0) {
        std::cout << "\n All HTTP_Request tests passed!\n";
    } else {
        std::cout << "\n" << failed << " tests failed\n";
    }
    return failed;
}
