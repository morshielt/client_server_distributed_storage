#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#include "exceptions.hpp"
#include "common.hpp"

#include <string>
#include <regex>
#include <iostream>
#include <cassert>

namespace excpt = sik_2::exceptions;
namespace cmmn = sik_2::common;

namespace sik_2::validation {
    namespace {
        // IP dot notation regex
        const std::string OCTET{"(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"};
        static const std::string IP{"^(?:" + OCTET + "\\.)" + "{3}" + OCTET + "$"};
    }

    // Check if IP was in a valid dot notation
    static bool validate_ip(const std::string &ip) {
        try {
            std::regex re{IP};
            std::cmatch cm;
            std::regex_match(ip.c_str(), cm, re);

            return !cm.empty();

        } catch (std::regex_error &e) {
            throw excpt::invalid_argument{"mcast_addr = " + ip};
        }
    }

    // Checks whethers numberic value is in range [min, max]
    template<typename T>
    bool in_range_inclusive(T value, T min, T max) {
        return (value >= min && value <= max);
    }
}
#endif //VALIDATION_HPP
