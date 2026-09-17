#include "../includes/HttpResponse.hpp"
#include "../includes/LocationConfig.hpp"
#include "../includes/ServerConfig.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>

namespace {

void expect(bool ok, const char* msg) {
    if (!ok) {
        std::cerr << "FAIL: " << msg << '\n';
        std::exit(1);
    }
}

std::string rootDir() {
    const char* env = std::getenv("HTTP_RESPONSE_FIXTURE_ROOT");
    return (env && *env) ? env : "tests/fixtures/www";
}

ServerConfig makeServer(const std::string& root) {
    ServerConfig server;
    server.setClientMaxBodySize(1024);
    server.addErrorPagePath(404, "/errors/404.html");

    LocationConfig rootLoc;
    rootLoc.setPath("/");
    rootLoc.setRoot(root);
    rootLoc.addIndex("index.html");
    rootLoc.addAllowMethods("GET");
    rootLoc.setAutoindex(true);
    server.addLocation(rootLoc);

    LocationConfig upload;
    upload.setPath("/upload");
    upload.setRoot(root + "/upload");
    upload.addAllowMethods("GET");
    upload.addAllowMethods("DELETE");
    server.addLocation(upload);

    LocationConfig redir;
    redir.setPath("/redirect");
    redir.setRedirection(301, "https://example.com/");
    redir.addAllowMethods("GET");
    server.addLocation(redir);

    LocationConfig cgi;
    cgi.setPath(".py");
    cgi.setCgiPass("/usr/bin/python3");
    cgi.addAllowMethods("GET");
    server.addLocation(cgi);

    return server;
}

HttpResponse::RequestView req(const std::string& method, const std::string& path,
                              const std::string& body = "") {
    HttpResponse::RequestView v;
    v.method = method;
    v.path = path;
    v.body = body;
    return v;
}

}  // namespace

int main() {
    const std::string root = rootDir();
    ServerConfig server = makeServer(root);
    HttpResponse response;

    expect(response.buildForPath(req("GET", "/"), server), "match /");
    expect(response.statusCode() == 200, "GET / 200");
    expect(response.getBody().find("fixture index") != std::string::npos, "index body");

    response.buildForPath(req("GET", "/missing.html"), server);
    expect(response.statusCode() == 404, "404");
    expect(response.getBody().find("custom 404") != std::string::npos, "custom 404 page");

    response.buildForPath(req("POST", "/"), server);
    expect(response.statusCode() == 405, "405");

    response.buildForPath(req("GET", "/", std::string(2048, 'x')), server);
    expect(response.statusCode() == 413, "413");

    response.buildForPath(req("GET", "/redirect"), server);
    expect(response.statusCode() == 301, "301");
    expect(response.getHeaders().at("Location") == "https://example.com/", "Location");

    response.buildForPath(req("GET", "/hello.py"), server);
    expect(response.needsCgi(), "cgi");
    expect(response.getRaw().empty(), "cgi raw empty");

    response.buildForPath(req("GET", "/empty/"), server);
    expect(response.statusCode() == 200, "autoindex");
    expect(response.getBody().find("Index of") != std::string::npos, "autoindex html");

    const std::string doomed = root + "/upload/to_delete.txt";
    {
        std::ofstream out(doomed.c_str());
        out << "bye\n";
    }
    response.buildForPath(req("DELETE", "/upload/to_delete.txt"), server);
    expect(response.statusCode() == 204, "DELETE 204");
    struct stat st {};
    expect(stat(doomed.c_str(), &st) != 0, "deleted");

    HttpResponse::RequestView bad = req("GET", "/");
    bad.errorStatus = 400;
    response.buildForPath(bad, server);
    expect(response.statusCode() == 400, "parser error");

    std::cout << "HttpResponse tests: OK\n";
    return 0;
}
