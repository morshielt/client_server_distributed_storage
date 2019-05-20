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
        const std::string DISCOVER{"[Dd][Ii][Ss][Cc][Oo][Vv][Ee][Rr]|d"};
        const std::string SEARCH{"[Ss][Ee][Aa][Rr][Cc][Hh]|s"};
        const std::string FETCH{"[Ff][Ee][Tt][Cc][Hh]|f"};
        const std::string UPLOAD{"[Uu][Pp][Ll][Oo][Aa][Dd]|u"};
        const std::string REMOVE{"[Rr][Ee][Mm][Oo][Vv][Ee]|r"};
        const std::string EXIT{"[Ee][Xx][Ii][Tt]|e"};

        const std::string ANY_END{"(?:\\s*((?:\\s*(?:.*))*))\\s*$"};
        const std::string OPTIONS{DISCOVER + "|" + SEARCH + "|" + FETCH + "|" + UPLOAD + "|" + REMOVE + "|" + EXIT};
    }

    class Request_parser {

    private:
        const std::string REQUEST{"^\\s*(" + OPTIONS + ")" + ANY_END};
        // indexes in cmatch
        static const size_t REQ{1};
        static const size_t PARAM{2};

        cmmn::Request match_request(const std::string &line, std::string &param) {
            try {
                std::regex re(REQUEST);
                std::cmatch cm;

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
                throw e;
            }
        }

        cmmn::Request recognise_request(std::string req, std::string param) { // validate?
            switch (req[0]) {
                case 'D':
                case 'd': {
                    if (param.empty()) return cmmn::Request::discover;
                    return cmmn::Request::unknown;
                }
                case 'S':
                case 's': {
                    return cmmn::Request::search;
                }
                case 'F':
                case 'f': {
                    if (!param.empty()) return cmmn::Request::fetch;
                    return cmmn::Request::unknown;
                }
                case 'U':
                case 'u': {
                    if (!param.empty()) return cmmn::Request::upload;
                    return cmmn::Request::unknown;
                }
                case 'R':
                case 'r': {
                    if (!param.empty()) return cmmn::Request::remove;
                    return cmmn::Request::unknown;
                }
                case 'E':
                case 'e': {
                    if (param.empty()) return cmmn::Request::exit;
                    return cmmn::Request::unknown;
                }
                default: {
                    return cmmn::Request::unknown;
                }
            }
        }

    public:
        cmmn::Request next_request(std::string &param) {
            std::string line;

            std::cout << "\nPlease enter request:\n";
            getline(std::cin, line);
            return match_request(line, param);
        }
    };
}
#endif //REQUEST_PARSER_HPP