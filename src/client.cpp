#include "client/client.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <string> // jest w iostream, ale to jest shady~
#include <cstdint>

namespace opt = boost::program_options;
namespace cmmn = sik_2::common;

int main(int argc, const char *argv[]) {

    std::string mcast_addr;
    int32_t cmd_port;
    std::string out_fldr;
    int32_t timeout;

    opt::options_description opt_desc("Options");
    opt::variables_map vm;

    try {
        opt_desc.add_options()
            ("help,h", "produce help message")
            (",g", opt::value<std::string>(&mcast_addr)->required(), "mcast_addr")
            (",p", opt::value<int32_t>(&cmd_port)->required(), "cmd_port")
            (",o", opt::value<std::string>(&out_fldr)->required(), "out_fldr")
            (",t", opt::value<int32_t>(&timeout)->default_value(cmmn::DEF_TIMEOUT), "timeout");

        opt::store(opt::parse_command_line(argc, argv, opt_desc), vm);
        opt::notify(vm);

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
        sik_2::client::client c{mcast_addr, cmd_port, out_fldr, timeout};
        c.run();

    } catch (std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        exit(1);
    }
}