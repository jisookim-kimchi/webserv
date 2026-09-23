#include "../includes/ConfigParser.hpp"
#include "test_utils.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

std::string g_argv0;

std::string fixture(const std::string& rel) {
    return projectRoot() + "/tests/fixtures/configs/" + rel;
}

std::string dumpServers(const std::vector<ServerConfig>& servers) {
    std::ostringstream out;

    for (size_t i = 0; i < servers.size(); ++i) {
        if (i > 0)
            out << '\n';
        out << "server [" << i << "]\n";
        out << "host    " << servers[i].getHost() << '\n';

        for (uint16_t port : servers[i].getPort())
            out << "listen    " << port << '\n';
        for (const std::string& name : servers[i].getServerName())
            out << "server_name     " << name << '\n';

        out << "client_max_body_size    " << servers[i].getClientMaxBodySize() << '\n';

        for (const auto& [code, path] : servers[i].getErrorPagePath())
            out << "error_page     " << code << ' ' << path << '\n';

        for (const LocationConfig& loc : servers[i].getLocations()) {
            out << "location    " << loc.getPath() << '\n';
            if (!loc.getRoot().empty())
                out << "root    " << loc.getRoot() << '\n';
            if (!loc.getCgiPass().empty())
                out << "cgi_pass    " << loc.getCgiPass() << '\n';
            out << "autoindex    " << (loc.getAutoindex() ? "on" : "off") << '\n';
            if (loc.getRedirection().first != 0)
                out << "return    " << loc.getRedirection().first << ' '
                    << loc.getRedirection().second << '\n';
            if (!loc.getAllowMethods().empty()) {
                out << "allow_methods    ";
                const auto& methods = loc.getAllowMethods();
                for (size_t m = 0; m < methods.size(); ++m) {
                    if (m != 0)
                        out << ' ';
                    out << methods[m];
                }
                out << '\n';
            }
            if (!loc.getIndex().empty()) {
                out << "index    ";
                const auto& indexes = loc.getIndex();
                for (size_t idx = 0; idx < indexes.size(); ++idx) {
                    if (idx != 0)
                        out << ' ';
                    out << indexes[idx];
                }
                out << '\n';
            }
        }
    }

    return out.str();
}

void test_dump_parser_diff() {
    ConfigParser parser;
    parser.parse(projectRoot() + "/configs/Basic.config");
    
    std::ofstream outfile(projectRoot() + "/tests/parseServer_parseLocation.txt");
    outfile << dumpServers(parser.getServerConfigs());
    outfile.close();
}

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    expectTrue(static_cast<bool>(in), "cannot open " + path);
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

// ConfigParser often calls exit(1). Re-exec this binary in --parse-exit mode so a
// failing parse cannot continue the parent test suite address space.
int runParseInChild(const std::string& path) {
    const pid_t pid = fork();
    expectTrue(pid >= 0, "fork failed");
    if (pid == 0) {
        execl(g_argv0.c_str(), g_argv0.c_str(), "--parse-exit", path.c_str(),
              static_cast<char*>(nullptr));
        _exit(127);
    }
    int status = 0;
    expectTrue(waitpid(pid, &status, 0) == pid, "waitpid failed");
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return -1;
}

void expectParseOk(const std::string& path) {
    const int code = runParseInChild(path);
    expectTrue(code == 0, "expected parse OK for " + path + " (exit=" + std::to_string(code) + ")");
}

void expectParseFails(const std::string& path) {
    const int code = runParseInChild(path);
    expectTrue(code != 0, "expected parse failure for " + path);
}

void writeGeneratedLargeConfig(const std::string& path, size_t servers, size_t locsPerServer) {
    std::ofstream out(path.c_str());
    expectTrue(static_cast<bool>(out), "cannot write " + path);
    out << "# generated large config: " << servers << " servers x " << locsPerServer
        << " locations\n";
    for (size_t s = 0; s < servers; ++s) {
        out << "server\n{\n";
        out << "    listen " << (20000 + s) << ";\n";
        if (s % 3 == 0)
            out << "    listen " << (30000 + s) << ";\n";
        out << "    host 127.0.0.1;\n";
        out << "    server_name host" << s << ".test alias" << s << ";\n";
        out << "    client_max_body_size " << ((s % 5) + 1) << "M;\n";
        out << "    error_page 404 /errors/" << s << ".html;\n";
        out << "    location /\n    {\n";
        out << "        root /data/s" << s << ";\n";
        out << "        index index.html;\n";
        out << "        allow_methods GET POST;\n";
        out << "        autoindex " << (s % 2 == 0 ? "on" : "off") << ";\n";
        out << "    }\n";
        for (size_t l = 0; l < locsPerServer; ++l) {
            out << "    location /loc" << l << "\n    {\n";
            out << "        root /data/s" << s << "/loc" << l << ";\n";
            out << "        allow_methods GET;\n";
            out << "        autoindex off;\n";
            if (l % 7 == 0)
                out << "        return 302 https://example.com/" << s << "/" << l << ";\n";
            if (l % 11 == 0)
                out << "        cgi_pass /usr/bin/python3;\n";
            out << "    }\n";
        }
        out << "}\n\n";
    }
}

void parse_test_config_structure() {
    ConfigParser parser;
    parser.parse(projectRoot() + "/configs/test.config");
    const auto& servers = parser.getServerConfigs();

    EXPECT_EQ(servers.size(), 1u);
    EXPECT_EQ(servers[0].getHost(), std::string("0.0.0.0"));
    EXPECT_EQ(servers[0].getPort().size(), 1u);
    EXPECT_EQ(servers[0].getPort()[0], 8080);
    EXPECT_EQ(servers[0].getServerName()[0], std::string("localhost"));
    EXPECT_TRUE(servers[0].getLocations().size() >= 4);

    bool foundCgi = false;
    bool foundUpload = false;
    for (const auto& loc : servers[0].getLocations()) {
        if (loc.getPath() == ".py") {
            foundCgi = true;
            EXPECT_CONTAINS(loc.getCgiPass(), "python");
        }
        if (loc.getPath() == "/upload")
            foundUpload = true;
    }
    EXPECT_TRUE(foundCgi);
    EXPECT_TRUE(foundUpload);
}

void parse_basic_config_snapshot() {
    ConfigParser parser;
    parser.parse(projectRoot() + "/configs/Basic.config");
    const std::string actual = dumpServers(parser.getServerConfigs());
    const std::string expected = readFile(projectRoot() + "/tests/expected/Basic.config.out");
    EXPECT_EQ(actual, expected);
}

void parse_edge_whitespace_and_comments() {
    ConfigParser parser;
    parser.parse(fixture("edge/whitespace_comments.config"));
    const auto& servers = parser.getServerConfigs();
    EXPECT_EQ(servers.size(), 2u);

    EXPECT_EQ(servers[0].getPort().size(), 2u);
    EXPECT_EQ(servers[0].getPort()[0], 9001);
    EXPECT_EQ(servers[0].getPort()[1], 9002);
    EXPECT_EQ(servers[0].getServerName().size(), 3u);
    EXPECT_EQ(servers[0].getClientMaxBodySize(), 512ULL * 1024ULL);
    EXPECT_TRUE(servers[0].getLocations().size() >= 3);

    EXPECT_EQ(servers[1].getPort()[0], 9003);
    EXPECT_EQ(servers[1].getHost(), std::string("0.0.0.0"));
    EXPECT_EQ(servers[1].getServerName()[0], std::string("packed"));
    EXPECT_EQ(servers[1].getClientMaxBodySize(), 1024ULL * 1024ULL * 1024ULL);
}

void parse_edge_dense_directives() {
    ConfigParser parser;
    parser.parse(fixture("edge/dense_directives.config"));
    const auto& servers = parser.getServerConfigs();
    EXPECT_EQ(servers.size(), 1u);
    EXPECT_EQ(servers[0].getPort().size(), 10u);
    EXPECT_EQ(servers[0].getServerName().size(), 26u);
    EXPECT_EQ(servers[0].getErrorPagePath().size(), 13u);
    EXPECT_EQ(servers[0].getLocations().size(), 11u);

    bool foundDeep = false;
    bool foundCgiPath = false;
    for (const auto& loc : servers[0].getLocations()) {
        if (loc.getPath() == "/a/b/c/d") {
            foundDeep = true;
            EXPECT_EQ(loc.getRoot(), std::string("/data/a/b/c/d"));
        }
        if (loc.getPath() == ".py") {
            foundCgiPath = true;
            EXPECT_CONTAINS(loc.getCgiPass(), "python");
        }
    }
    EXPECT_TRUE(foundDeep);
    EXPECT_TRUE(foundCgiPath);
}

void parse_edge_body_size_units() {
    ConfigParser parser;
    parser.parse(fixture("edge/body_size_units.config"));
    const auto& servers = parser.getServerConfigs();
    EXPECT_EQ(servers.size(), 4u);
    EXPECT_EQ(servers[0].getClientMaxBodySize(), 2ULL * 1024ULL);
    EXPECT_EQ(servers[1].getClientMaxBodySize(), 3ULL * 1024ULL * 1024ULL);
    EXPECT_EQ(servers[2].getClientMaxBodySize(), 1ULL * 1024ULL * 1024ULL * 1024ULL);
    EXPECT_EQ(servers[3].getClientMaxBodySize(), 4096ULL);
}

void parse_edge_empty_and_comments_only() {
    {
        ConfigParser parser;
        parser.parse(fixture("edge/empty.config"));
        EXPECT_EQ(parser.getServerConfigs().size(), 0u);
    }
    {
        ConfigParser parser;
        parser.parse(fixture("edge/comments_only.config"));
        EXPECT_EQ(parser.getServerConfigs().size(), 0u);
    }
}

void parse_large_generated_dataset() {
    const std::string path = projectRoot() + "/tests/fixtures/configs/edge/generated_large.config";
    const size_t nServers = 40;
    const size_t nLocs = 25;
    writeGeneratedLargeConfig(path, nServers, nLocs);

    ConfigParser parser;
    parser.parse(path);
    const auto& servers = parser.getServerConfigs();
    EXPECT_EQ(servers.size(), nServers);
    EXPECT_EQ(servers[0].getPort()[0], 20000);
    EXPECT_EQ(servers[nServers - 1].getPort()[0], static_cast<uint16_t>(20000 + nServers - 1));
    EXPECT_EQ(servers[7].getLocations().size(), nLocs + 1);
    EXPECT_EQ(servers[0].getClientMaxBodySize(), 1ULL * 1024ULL * 1024ULL);
    EXPECT_EQ(servers[4].getClientMaxBodySize(), 5ULL * 1024ULL * 1024ULL);

    bool foundRedirect = false;
    for (const auto& loc : servers[3].getLocations()) {
        if (loc.getPath() == "/loc0") {
            foundRedirect = true;
            EXPECT_EQ(loc.getRedirection().first, 302);
        }
    }
    EXPECT_TRUE(foundRedirect);
}

void parse_missing_file_throws() {
    bool threw = false;
    try {
        ConfigParser parser;
        parser.parse(fixture("edge/does_not_exist.config"));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

void parse_empty_filename_throws() {
    bool threw = false;
    try {
        ConfigParser parser;
        parser.parse("");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

void parse_invalid_configs_fail() {
    const char* bad[] = {
        "invalid/unclosed_server.config",
        "invalid/unclosed_location.config",
        "invalid/unknown_server_kw.config",
        "invalid/unknown_location_kw.config",
        "invalid/not_server_block.config",
        "invalid/missing_server_brace.config",
        "invalid/location_no_path.config",
        "invalid/random_prose.config",
        "invalid/bad_listen_port.config",
        "invalid/binary_garbage.config",
        "invalid/truncated.config",
    };
    for (const char* rel : bad)
        expectParseFails(fixture(rel));
}

void parse_valid_fixtures_ok_in_child() {
    expectParseOk(projectRoot() + "/configs/Basic.config");
    expectParseOk(projectRoot() + "/configs/test.config");
    expectParseOk(fixture("edge/whitespace_comments.config"));
    expectParseOk(fixture("edge/dense_directives.config"));
    expectParseOk(fixture("edge/body_size_units.config"));
}

}  // namespace

int main(int argc, char** argv) {
    g_argv0 = (argc > 0 && argv[0] != nullptr) ? argv[0] : "./build/test_config";

    if (argc >= 3 && std::strcmp(argv[1], "--parse-exit") == 0) {
        int nullFd = open("/dev/null", O_WRONLY);
        if (nullFd >= 0) {
            dup2(nullFd, STDOUT_FILENO);
            dup2(nullFd, STDERR_FILENO);
            if (nullFd > STDERR_FILENO)
                close(nullFd);
        }
        try {
            ConfigParser parser;
            parser.parse(argv[2]);
            return 0;
        } catch (...) {
            return 2;
        }
    }

    std::cout << "== config ==\n";
    int failed = 0;
    failed += runTest("parse_test_config_structure", parse_test_config_structure);
    failed += runTest("parse_basic_config_snapshot", parse_basic_config_snapshot);
    failed += runTest("test_dump_parser_diff", test_dump_parser_diff);
    failed += runTest("parse_edge_whitespace_and_comments", parse_edge_whitespace_and_comments);
    failed += runTest("parse_edge_dense_directives", parse_edge_dense_directives);
    failed += runTest("parse_edge_body_size_units", parse_edge_body_size_units);
    failed += runTest("parse_edge_empty_and_comments_only", parse_edge_empty_and_comments_only);
    failed += runTest("parse_large_generated_dataset", parse_large_generated_dataset);
    failed += runTest("parse_missing_file_throws", parse_missing_file_throws);
    failed += runTest("parse_empty_filename_throws", parse_empty_filename_throws);
    failed += runTest("parse_invalid_configs_fail", parse_invalid_configs_fail);
    failed += runTest("parse_valid_fixtures_ok_in_child", parse_valid_fixtures_ok_in_child);

    const int total = 12;
    std::cout << "-- config: " << (total - failed) << "/" << total << " passed\n";
    return failed == 0 ? 0 : 1;
}
