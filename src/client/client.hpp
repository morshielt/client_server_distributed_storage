#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../common/common.hpp"
#include "request_parser.hpp"
#include <string>
#include <iostream>
#include <regex>

namespace sik_2::client {

    class client {
    private:
        std::string mcast_addr;
        int32_t cmd_port;
        std::string out_fldr;
        int32_t timeout;

        sik_2::client::request_parser req_parser;

    public:
        client() = delete;
        client(const client &other) = delete;
        client(client &&other) = delete;

        client(std::string mcast_addr, const int16_t cmd_port, std::string out_fldr, int16_t timeout) :
            mcast_addr{mcast_addr}, cmd_port{cmd_port}, out_fldr{out_fldr}, timeout{timeout} {
            // if (cmmn::valid_ip(mcast_addr)) {
            //     c
            // }

            std::cout << cmmn::valid_ip(mcast_addr) << "\n";
        };


        void run() {
            while (true) {
                req_parser.next_request();
            }
        }
    };
}

#endif //CLIENT_HPP
