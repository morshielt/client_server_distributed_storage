#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include <cstdint>
#include <array>
#include <cstring>
#include "common.hpp"

namespace cmmn = sik_2::common;

namespace sik_2::commands {

    namespace {
        void print_bytes(int size, const char *str) {
            for (int i = 0; i < size; ++i) {
                printf("%d ", (const unsigned char) str[i]);
            }
            printf("\n");
        }

        static const int32_t MAX_UDP_PACKET_SIZE{65507};
        static const size_t CMD_SIZE = 10;
    }

    class Cmd {
    protected:
        std::array<char, MAX_UDP_PACKET_SIZE> msg;
        std::size_t data_offset;
        std::size_t size;

    public:
        Cmd(size_t data_offset) : data_offset{data_offset} {}

        char *raw_msg() {
            return msg.data();
        }

        size_t msg_size() {
            return size;
        }

    };

    class Cmplx_cmd : public Cmd {
    public:
        Cmplx_cmd(const char *cmd, uint64_t cmd_seq, uint64_t param, const char *data, size_t data_size)
            : Cmd(CMD_SIZE + 2 * sizeof(uint64_t)) {

            size = data_offset + data_size;
            // std::cout << "cmd " << cmd << "\n";
            // std::cout << "cmd_seq " << cmd_seq << "\n";
            // std::cout << "data " << data << "\n";
            // std::cout << "data_size " << data_size << "\n";

            memcpy(msg.data(), cmd, strlen(cmd));
            memset(msg.data() + strlen(cmd), '\0', CMD_SIZE - strlen(cmd));

            uint64_t be_cmd_seq = htobe64(cmd_seq);
            memcpy(msg.data() + CMD_SIZE, &be_cmd_seq, sizeof(uint64_t));

            uint64_t be_param = htobe64(param);
            memcpy(msg.data() + CMD_SIZE + sizeof(uint64_t), &be_cmd_seq, sizeof(uint64_t));

            memcpy(msg.data() + data_offset, data, data_size);

            if (cmmn::DEBUG) print_bytes(data_offset + data_size, msg.data());
        }
    };


    // PLAN TESTÓW - nie harmonogram,
    // tylko jakiego rodzaju tetsy i w jaki sposób należy i chcielisbyśmy wukonać dla naszego porjektu
    // Uwaga : nie wymaga żeby tetsy opisane robić, byłoby miło jakby zrobić chociaż część,
    // ale nie jest to wymagane,
    // lepiej żeby plan był pełny i sensowny, a nie wykonany xD
    //
    // nie scenariusze testowe, chodzi o opisanie wszystkich rodzajów tetsów które warto/należoałoby wykonywać dla
    // naszych systemów
    // testy tego przy użyciu tego ( testy cczegoś przy użyciu automatycznych skryptów w bashu/ człowieka / selenium)
    // 1/2/3 strony, wyczerpujące ma być~

    // we wtorek nie będzie grzesia znowu


    class Simpl_cmd : public Cmd {
    public:
        Simpl_cmd(const char *cmd, uint64_t cmd_seq, const char *data, size_t data_size)
            : Cmd(CMD_SIZE + sizeof(uint64_t)) {

            size = data_offset + data_size;

            memcpy(msg.data(), cmd, strlen(cmd));
            memset(msg.data() + strlen(cmd), '\0', CMD_SIZE - strlen(cmd));

            uint64_t be = htobe64(cmd_seq);
            memcpy(msg.data() + CMD_SIZE, &be, sizeof(uint64_t));

            memcpy(msg.data() + data_offset, data, data_size);

            if (cmmn::DEBUG) print_bytes(data_offset + data_size, msg.data());

        }

    };
}

#endif //COMMANDS_HPP
