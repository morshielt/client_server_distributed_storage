#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <regex>
#include <iostream>
#include <cassert>

namespace sik_2::common {
    static const int DEBUG = 1;
    static const int DEF_TIMEOUT = 5;

    namespace {
        //IP REGEX ((?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?))
        // TODO z "^" i "$" czy bez?
        std::string OCTET{"(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"};
        std::string IP{"^(?:" + OCTET + "\\.)" + "{3}" + OCTET + "$"};
    }

    bool valid_ip(const std::string &ip) {
        try {
            std::regex re(IP);
            std::cmatch cm;    // same as std::match_results<const char*> cm;

            std::regex_match(ip.c_str(), cm, re);

            if (DEBUG) {
                std::cout << "string literal with " << cm.size() << " matches\n";

                std::cout << "the matches were: ";
                for (const auto &i : cm) {
                    std::cout << "[" << i << "] ";
                }
                std::cout << "\n";
            }
            assert(cm.size() == 1 || cm.size() == 0);
            return cm.empty();

        } catch (std::regex_error &e) {
            // Syntax error in the regular expression
            throw e;
        }
    }
}
#endif //COMMON_HPP
