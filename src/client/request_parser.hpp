#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include <string>
#include <regex>
#include <iostream>

namespace cmmn = sik_2::common;

namespace sik_2::request_parser {

    // regexes
    namespace {
        const std::string DISCOVER{"[Dd][Ii][Ss][Cc][Oo][Vv][Ee][Rr]"};
        const std::string SEARCH{"[Ss][Ee][Aa][Rr][Cc][Hh]"};
        const std::string FETCH{"[Ff][Ee][Tt][Cc][Hh]"};
        const std::string UPLOAD{"[Uu][Pp][Ll][Oo][Aa][Dd]"};
        const std::string REMOVE{"[Rr][Ee][Mm][Oo][Vv][Ee]"};
        const std::string EXIT{"[Ee][Xx][Ii][Tt]"};

        const std::string ANY_END{"(?:\\s*|\\s((?:\\s*(?:.*))*))\\s*$"};
        const std::string OPTIONS{DISCOVER + "|" + SEARCH + "|" + FETCH + "|" + UPLOAD + "|" + REMOVE + "|" + EXIT};
    }

    // Clinets command line requests parser
    class request_parser {

    public:
        // Get whole line and try to match it
        cmmn::Request next_request(std::string &param) {
            std::string line;

            getline(std::cin, line);
            return match_request(line, param);
        }

    private:
        const std::string REGEX{"^\\s*(" + OPTIONS + ")" + ANY_END};
        // Indexes in cmatch
        static const size_t REQ{1};
        static const size_t PARAM{2};

        // Try match to regexes
        cmmn::Request match_request(const std::string &line, std::string &param) {
            try {
                std::regex regex(REGEX);
                std::cmatch match_results;

                std::regex_match(line.c_str(), match_results, regex);
                param = match_results[PARAM];
                return recognise_request(match_results[REQ], match_results[PARAM]);

            } catch (std::regex_error &e) {
                throw e;
            }
        }

        // Request was valid, choose information for client
        cmmn::Request recognise_request(const std::string &req, const std::string &param) {
            switch (req[0]) {
                case 'D': case 'd': {
                    if (param.empty()) return cmmn::Request::discover;
                    break;

                }
                case 'S': case 's': {
                    return cmmn::Request::search;

                }
                case 'F': case 'f': {
                    if (!param.empty()) return cmmn::Request::fetch;
                    break;

                }
                case 'U': case 'u': {
                    if (!param.empty()) return cmmn::Request::upload;
                    break;

                }
                case 'R': case 'r': {
                    if (!param.empty()) return cmmn::Request::remove;
                    break;

                }
                case 'E': case 'e': {
                    if (param.empty()) return cmmn::Request::exit;
                    break;
                }
            }
            return cmmn::Request::unknown;
        }
    };
}
#endif //REQUEST_PARSER_HPP