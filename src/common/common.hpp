#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <regex>
#include <iostream>
#include <cassert>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace sik_2::common {

    static const int DEBUG{1};
    static const int32_t DEF_TIMEOUT{5};
    static const int32_t MAX_TIMEOUT{300};
    static const int32_t MAX_PORT{65535};
    static const int32_t TTL_VALUE{4};
    static const int32_t MAX_UDP_PACKET_SIZE{65507};
    static const size_t CMD_SIZE = 10;

    // commands
    static const char *hello_ = "HELLO";
    static const char *good_day_ = "GOOD_DAY";
    static const char *list_ = "LIST";
    static const char *my_list_ = "MY_LIST";
    static const char *get_ = "GET";
    static const char *connect_me_ = "CONNECT_ME";
    static const char *add_ = "ADD";
    static const char *no_way_ = "NO_WAY";
    static const char *can_add_ = "CAN_ADD";

    void syserr(const std::string &msg, const int &line, const std::string &file) {
        std::cerr << msg << " " << std::to_string(line) << " : " << file << "\n";
    }

    void print_bytes(int size, const char *str) {

        for (int j = 0; j < CMD_SIZE; ++j) {
            printf("%c ", (const unsigned char) str[j]);
        }

        for (int i = CMD_SIZE; i < size; ++i) {
            printf("%d ", (const unsigned char) str[i]);
        }
        printf("\n");
    }

}
#endif //COMMON_HPP
