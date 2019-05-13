//
// Created by maria on 12.05.19.
//

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../common/common.hpp"
#include "request_parser.hpp"
#include <string>
#include <iostream>
#include <regex>

namespace sik2::client {

    class client {
    private:
        std::string MCAST_ADDR;
        sik2::client::request_parser req_parser;

        //IP REGEX ((?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?))

    public:
        // client() = default;

        void run() {
            while (true) {
                req_parser.next_request();
            }
        }
    };
}

#endif //CLIENT_HPP
