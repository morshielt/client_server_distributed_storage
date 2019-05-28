#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include <string>
#include <regex>
#include <iostream>

namespace cmmn = sik_2::common;

namespace sik_2::request_parser {

    // regexes
    namespace {
        const std::string DISCOVER{"[Dd][Ii][Ss][Cc][Oo][Vv][Ee][Rr]|d"};
        const std::string SEARCH{"[Ss][Ee][Aa][Rr][Cc][Hh]|s"};
        const std::string FETCH{"[Ff][Ee][Tt][Cc][Hh]|f"};
        const std::string UPLOAD{"[Uu][Pp][Ll][Oo][Aa][Dd]|u"};
        const std::string REMOVE{"[Rr][Ee][Mm][Oo][Vv][Ee]|r"};
        const std::string EXIT{"[Ee][Xx][Ii][Tt]|e"};

        const std::string ANY_END{"(?:\\s*|\\s((?:\\s*(?:.*))*))\\s*$"};
        const std::string OPTIONS{DISCOVER + "|" + SEARCH + "|" + FETCH + "|" + UPLOAD + "|" + REMOVE + "|" + EXIT};
    }

    class request_parser {

    public:
        cmmn::Request next_request(std::string &param) {
            std::string line;

            std::cout << "Please enter request:\n";
            getline(std::cin, line);
            return match_request(line, param);
        }

    private:
        const std::string REGEX{"^\\s*(" + OPTIONS + ")" + ANY_END};
        static const size_t REQ{1};
        static const size_t PARAM{2};

        cmmn::Request match_request(const std::string &line, std::string &param) {
            try {
                std::regex regex(REGEX);
                std::cmatch match_results;

                std::regex_match(line.c_str(), match_results, regex);

                if (cmmn::DEBUG) {
                    for (const auto &i : match_results) {
                        std::cout << "[" << i << "] ";
                    }
                    std::cout << "\n";
                }

                param = match_results[PARAM];
                return recognise_request(match_results[REQ], match_results[PARAM]);

            } catch (std::regex_error &e) {
                throw e;
            }
        }

        cmmn::Request recognise_request(const std::string &req, const std::string &param) {
            switch (req[0]) {
                case 'D': case 'd': {
                    if (param.empty()) return cmmn::Request::discover;
                    break;

                } case 'S': case 's': {
                    return cmmn::Request::search;

                } case 'F': case 'f': {
                    if (!param.empty()) return cmmn::Request::fetch;
                    break;

                } case 'U': case 'u': {
                    if (!param.empty()) return cmmn::Request::upload;
                    break;

                } case 'R': case 'r': {
                    if (!param.empty()) return cmmn::Request::remove;
                    break;

                } case 'E': case 'e': {
                    if (param.empty()) return cmmn::Request::exit;
                    break;
                }
            }
            return cmmn::Request::unknown;
        }
    };
}
#endif //REQUEST_PARSER_HPP