#include "server/server.hpp"

#include "client/Client.hpp"
#include <boost/program_options.hpp>

#include <iostream>
#include <string> // jest w iostream, ale to jest shady~
#include <cstdint>

namespace opt = boost::program_options;
namespace cmmn = sik_2::common;

namespace {
    const int DEF_MAX_SPACE = 52428800;
}

int main(int argc, const char *argv[]) {

    std::string mcast_addr;
    int32_t cmd_port;
    std::string shdr_fldr;
    int32_t timeout;

    opt::options_description opt_desc("Options");
    opt::variables_map vm;

    try {
        opt_desc.add_options()
            ("help,h", "produce help message")
            (",g", opt::value<std::string>(&mcast_addr)->required(), "mcast_addr")
            (",p", opt::value<int32_t >(&cmd_port)->required(), "cmd_port")
            (",f", opt::value<std::string>(&shdr_fldr)->required(), "shdr_fldr")
            (",t", opt::value<int32_t>(&timeout)->default_value(cmmn::DEF_TIMEOUT), "timeout");

        opt::store(opt::parse_command_line(argc, argv, opt_desc), vm);
        opt::notify(vm);

        if (cmmn::DEBUG) std::cout << "mcast_addr " << mcast_addr << "\n";
        if (cmmn::DEBUG) std::cout << "cmd_port " << cmd_port << "\n";
        if (cmmn::DEBUG) std::cout << "shdr_fldr " << shdr_fldr << "\n";
        if (cmmn::DEBUG) std::cout << "timeout " << timeout << "\n";

    } catch (const std::exception &e) {
        if (vm.count("help")) {
            std::cout << opt_desc << "\n";
            return 1;
        } else {
            std::cerr << "ERROR: " << e.what() << "\n";
            exit(1);
        }
    }

    try {

    } catch (std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        exit(1);
    }
}