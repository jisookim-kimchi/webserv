#include "../includes/ConfigParser.hpp"
#include <fstream>
#include <iostream>

void dumpParseServerLocation(const std::string& configPath, const std::string& outputPath) {
    ConfigParser parser;
    parser.parse(configPath);
    const std::vector<ServerConfig>& Configs = parser.getServerConfigs();
    std::ofstream outfile(outputPath);
    if (!outfile.is_open()) {
        std::cerr << "error : cannot open output file " << outputPath << "\n";
        return;
    }
    for (size_t i = 0; i < Configs.size(); i++) {
        if (0 < i)
            outfile << "\n";
        outfile << "server [" << i << "]\n";
        outfile << "host    " << Configs[i].getHost() << '\n';
        const std::vector<uint16_t>& ports = Configs[i].getPort();
        for (size_t j = 0; j < ports.size(); j++) {
            outfile << "listen    " << ports[j] << '\n';
        }
        const std::vector<std::string>& names = Configs[i].getServerName();
        for (size_t j = 0; j < names.size(); j++) {
            outfile << "server_name     " << names[j] << '\n';
        }
        outfile << "client_max_body_size    " << Configs[i].getClientMaxBodySize() << '\n';
        const std::map<uint16_t, std::string>& errs = Configs[i].getErrorPagePath();
        for (const auto& [key, value] : errs) {
            outfile << "error_page     " << key << " " << value << '\n';
        }
        const std::vector<LocationConfig>& locs = Configs[i].getLocations();
        for (size_t k = 0; k < locs.size(); k++) {
            outfile << "location    " << locs[k].getPath() << '\n';
            if (!locs[k].getRoot().empty())
                outfile << "root    " << locs[k].getRoot() << '\n';
            if (!locs[k].getCgiPass().empty())
                outfile << "cgi_pass    " << locs[k].getCgiPass() << '\n';
            outfile << "autoindex    " << (locs[k].getAutoindex() ? "on" : "off") << '\n';
            if (locs[k].getRedirection().first != 0)
                outfile << "return    " << locs[k].getRedirection().first << " "
                        << locs[k].getRedirection().second << '\n';
            if (!locs[k].getAllowMethods().empty()) {
                outfile << "allow_methods    ";
                const std::vector<std::string>& methods = locs[k].getAllowMethods();
                for (size_t m = 0; m < methods.size(); ++m) {
                    if (m != 0)
                        outfile << ' ';
                    outfile << methods[m];
                }
                outfile << '\n';
            }
            if (!locs[k].getIndex().empty()) {
                outfile << "index    ";
                const std::vector<std::string>& indexes = locs[k].getIndex();
                for (size_t idx = 0; idx < indexes.size(); ++idx) {
                    if (idx != 0)
                        outfile << ' ';
                    outfile << indexes[idx];
                }
                outfile << '\n';
            }
        }
    }
    outfile.close();
}