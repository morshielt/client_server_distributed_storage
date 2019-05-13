#ifndef STRUCTS_HPP
#define STRUCTS_HPP
#include <cstdint>

struct simpl_cmd {
    char cmd[10];
    uint64_t cmd_seq;
    char data[];
};

struct cmplx_cmd {
    char cmd[10];
    uint64_t cmd_seq;
    uint64_t param;
    char data[];
};

#endif //STRUCTS_HPP
