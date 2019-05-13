//
// Created by maria on 12.05.19.
//

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <iostream>
#include <regex>

namespace sik2::client {
    namespace {
        std::string DISCOVER = "[Dd][Ii][Ss][Cc][Oo][Vv][Ee][Rr]";
        std::string SEARCH = "[Ss][Ee][Aa][Rr][Cc][Hh]";
        std::string FETCH = "[Ff][Ee][Tt][Cc][Hh]";
        std::string UPLOAD = "[Uu][Pp][Ll][Oo][Aa][Dd]";
        std::string REMOVE = "[Rr][Ee][Mm][Oo][Vv][Ee]";
        std::string EXIT = "[Ee][Xx][Ii][Tt]";

        std::string ANY_END = "(?:\\s*((?:\\s*(?:\\w|\\/))*))\\s*$";
        std::string OPTIONS = DISCOVER + "|" + SEARCH + "|" + FETCH + "|" + UPLOAD + "|" + REMOVE + "|" + EXIT;
        std::string REQUEST = "^\\s*(" + OPTIONS + ")" + ANY_END;

        // indexes in cmatch
        const size_t REQ = 1;
        const size_t PARAM = 2;
    }

    class client {
    public:
        std::string MCAST_ADDR;

        //IP REGEX ((?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?))
        void recoginse_request() {
            std::string subject;

            while (true) {
                std::cout << "Please enter request:\n";
                getline(std::cin, subject);

                // std::string subject("This is a test");
                // try {
                //     std::regex re(DISCOVER);
                //     std::sregex_iterator next(subject.begin(), subject.end(), re);
                //     std::sregex_iterator end;
                //     while (next != end) {
                //         std::smatch match = *next;
                //         std::cout << "\"" << match.str() << "\"" << "\n";
                //         next++;
                //     }

                std::regex re(REQUEST);
                std::cmatch cm;    // same as std::match_results<const char*> cm;
                std::regex_match(subject.c_str(), cm, re);
                std::cout << "string literal with " << cm.size() << " matches\n";

                std::cout << "the matches were: ";
                for (unsigned i = 0; i < cm.size(); ++i) {
                    std::cout << "[" << cm[i] << "] ";
                }

                // } catch (std::regex_error &e) {
                //     throw e;
                //
                //     // Syntax error in the regular expression
                // }
            }
        }

    public:
        client() = default;
    };
}

#endif //CLIENT_HPP
