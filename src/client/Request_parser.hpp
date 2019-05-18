#pragma clang diagnostic push
#pragma ide diagnostic ignored "MemberFunctionCanBeStaticInspection"
#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include <string>
#include <regex>
#include <iostream>

namespace cmmn = sik_2::common;

namespace sik_2::request_parser {
    namespace {
        const std::string DISCOVER{"[Dd][Ii][Ss][Cc][Oo][Vv][Ee][Rr]"};
        const std::string SEARCH{"[Ss][Ee][Aa][Rr][Cc][Hh]"};
        const std::string FETCH{"[Ff][Ee][Tt][Cc][Hh]"};
        const std::string UPLOAD{"[Uu][Pp][Ll][Oo][Aa][Dd]"};
        const std::string REMOVE{"[Rr][Ee][Mm][Oo][Vv][Ee]"};
        const std::string EXIT{"[Ee][Xx][Ii][Tt]"};

        const std::string ANY_END{"(?:\\s*((?:\\s*(?:\\w|\\/))*))\\s*$"};
        const std::string OPTIONS{DISCOVER + "|" + SEARCH + "|" + FETCH + "|" + UPLOAD + "|" + REMOVE + "|" + EXIT};
    }

    enum class Request { search, discover, fetch, upload, remove, exit, unknown };

    class Request_parser {
    private:
        const std::string REQUEST{"^\\s*(" + OPTIONS + ")" + ANY_END};
        // indexes in cmatch
        static const size_t REQ{1};
        static const size_t PARAM{2};

        // std::string param;

        Request match_request(const std::string &line, std::string &param) {
            try {
                std::regex re(REQUEST);
                std::cmatch cm;    // same as std::match_results<const char*> cm;

                std::regex_match(line.c_str(), cm, re);

                if (cmmn::DEBUG) {
                    std::cout << "string literal with " << cm.size() << " matches\n";

                    std::cout << "the matches were: ";
                    for (const auto &i : cm) {
                        std::cout << "[" << i << "] ";
                    }
                    std::cout << "\n";
                }

                param = cm[PARAM];
                return recognise_request(cm[REQ], cm[PARAM]);

            } catch (std::regex_error &e) {
                // syntax error in regex
                throw e;
            }
        }

        Request recognise_request(std::string req, std::string param) { // validate?
            switch (req[0]) {
                case 'D':
                case 'd': {
                    // if (cmmn::DEBUG) std::cout << "DISCOVER" << "\n";
                    if (param.empty()) return Request::discover;
                    break;
                }
                case 'S':
                case 's': {
                    // if (cmmn::DEBUG) std::cout << "SEARCH" << "\n";
                    break;
                }
                case 'F':
                case 'f': {
                    if (param.empty()) return Request::discover;

                    // if (cmmn::DEBUG) std::cout << "FETCH" << "\n";
                    break;
                }
                case 'U':
                case 'u': {
                    // if (cmmn::DEBUG) std::cout << "UPLOAD" << "\n";
                    break;
                }
                case 'R':
                case 'r': {
                    // if (cmmn::DEBUG) std::cout << "REMOVE" << "\n";
                    break;
                }
                case 'E':
                case 'e': {
                    // if (cmmn::DEBUG) std::cout << "EXIT" << "\n";
                    break;
                }
                default: {
                    std::cout << "UNKNOWN : " << req << "\n";
                }
            }
        }

    public:
        Request next_request(std::string &param) {
            std::string line;

            std::cout << "\nPlease enter request:\n";
            getline(std::cin, line);
            return match_request(line, param);
        }
    };
}
#endif //REQUEST_PARSER_HPP