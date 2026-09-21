#include "../includes/ConfigParser.hpp"
#include "test_utils.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

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

void parse_test_config_structure() {
    ConfigParser parser;
    parser.parse(projectRoot() + "/configs/test.config");
    const auto& servers = parser.getServerConfigs();

    EXPECT_EQ(servers.size(), 1u);
    EXPECT_EQ(servers[0].getHost(), std::string("127.0.0.1"));
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

}  // namespace

int main() {
    std::cout << "== config ==\n";
    int failed = 0;
    failed += runTest("parse_test_config_structure", parse_test_config_structure);
    failed += runTest("parse_basic_config_snapshot", parse_basic_config_snapshot);
    failed += runTest("test_dump_parser_diff", test_dump_parser_diff);
    std::cout << "-- config: " << (3 - failed) << "/3 passed\n";
    return failed == 0 ? 0 : 1;
}
