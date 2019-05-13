#include "client/client.hpp"
// #include <boost/program_options.hpp>
#include <iostream>
#include <string> // jest w iostream, ale to jest shady~
#include <cstdint>

// namespace opt = boost::program_options;

static const int DEF_TIMEOUT = 5;

int main(int argc, const char *argv[]) {

    // opt::options_description opt_desc("Options");
    // opt::variables_map vm;
    //
    // std::string MCAST_ADDR;
    // int32_t CMD_PORT;
    // std::string OUT_FLDR;
    // int32_t TIMEOUT;

    // try {
    //     opt_desc.add_options()
    //         ("help,h", "produce help message")
    //         ("g", opt::value<std::string>(&MCAST_ADDR)->required(), "MCAST_ADDR")
    //         ("p", opt::value<int32_t>(&CMD_PORT)->required(), "CMD_PORT")
    //         ("o", opt::value<std::string>(&OUT_FLDR)->required(), "OUT_FLDR")
    //         ("t", opt::value<int32_t>(&TIMEOUT)->default_value(DEF_TIMEOUT), "TIMEOUT");
    //
    //     opt::store(opt::parse_command_line(argc, argv, opt_desc), vm);
    //     opt::notify(vm);
    //
    //     std::cout << "MCAST_ADDR " << MCAST_ADDR << "\n";
    //     std::cout << "CMD_PORT " << CMD_PORT << "\n";
    //     std::cout << "OUT_FLDR " << OUT_FLDR << "\n";
    //     std::cout << "TIMEOUT " << TIMEOUT << "\n";
    //
    // } catch (const boost::program_options::required_option &e) {
    //     if (vm.count("help")) {
    //         std::cout << opt_desc << std::endl;
    //         return 1;
    //     } else {
    //         throw e;
    //     }
    // }

    sik2::client::client c = sik2::client::client();
    c.run();

}