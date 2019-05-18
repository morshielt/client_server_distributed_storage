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
        // TODO z "^" i "$" czy bez?
        const std::string OCTET{"(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"};
        static const std::string IP{"^(?:" + OCTET + "\\.)" + "{3}" + OCTET + "$"};
    }

    static bool valid_ip(const std::string &ip) {
        try {
            std::regex re{IP};
            std::cmatch cm;

            std::regex_match(ip.c_str(), cm, re);

            if (cmmn::DEBUG) {
                std::cout << "string literal with " << cm.size() << " matches\n";

                std::cout << "the matches were: ";
                for (const auto &i : cm) {
                    std::cout << "[" << i << "] ";
                }
                std::cout << "\n";
            }

            assert(cm.size() == 1 || cm.size() == 0);
            return !cm.empty();

        } catch (std::regex_error &e) { // XXX to jest takie FYI ~ propagacja
            // syntax error in regex
            throw;
        }
    }

    // TODO  wywala jak jest permission denied, czy słusznie?
    static bool valid_directory(std::string path) {
        try {
            if (cmmn::DEBUG) {
                const fs::file_status file = fs::status(path);
                std::cout << "file.type() " << file.type() << "\n";
                std::cout << "file.permissions() " << file.permissions() << "\n";
            }

            fs::directory_iterator i(path); // działa jak próba otworzenia, wywala perm denied jak trzeba
            return fs::is_directory(path);
        } catch (...) {
            throw;
        }
    }

    template<typename T>
    bool in_range_incl(T value, T min, T max) {
        return (value >= min && value <= max);
    }
}
#endif //VALIDATION_HPP
