#include "client/client.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <string> // jest w iostream, ale to jest shady~
#include <cstdint>

namespace opt = boost::program_options;
namespace cmmn = sik_2::common;

int main(int argc, const char *argv[]) {

    opt::options_description opt_desc("Options");
    opt::variables_map vm;

    std::string mcast_addr;
    int32_t cmd_port;
    std::string out_fldr;
    int32_t timeout;

    try {
        opt_desc.add_options()
            ("help,h", "produce help message")
            ("g", opt::value<std::string>(&mcast_addr)->required(), "mcast_addr")
            ("p", opt::value<int32_t>(&cmd_port)->required(), "cmd_port")
            ("o", opt::value<std::string>(&out_fldr)->required(), "out_fldr")
            ("t", opt::value<int32_t>(&timeout)->default_value(cmmn::DEF_TIMEOUT), "timeout");

        opt::store(opt::parse_command_line(argc, argv, opt_desc), vm);
        opt::notify(vm);

        if (cmmn::DEBUG) std::cout << "mcast_addr " << mcast_addr << "\n";
        if (cmmn::DEBUG) std::cout << "cmd_port " << cmd_port << "\n";
        if (cmmn::DEBUG) std::cout << "out_fldr " << out_fldr << "\n";
        if (cmmn::DEBUG) std::cout << "timeout " << timeout << "\n";

    } catch (const boost::program_options::required_option &e) {
        if (vm.count("help")) {
            std::cout << opt_desc << std::endl;
            return 1;
        } else {
            throw e;
        }
    }

    sik_2::client::client c = sik_2::client::client(
        vm["g"].as<std::string>(),
        vm["p"].as<int32_t>(),
        vm["o"].as<std::string>(),
        vm["t"].as<int32_t>()
    );
    c.run();

}