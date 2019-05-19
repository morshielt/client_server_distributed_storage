#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include <cstdint>
#include <array>
#include <cstring>
#include "common.hpp"

namespace cmmn = sik_2::common;

namespace sik_2::commands {

    class Simpl_cmd {
    protected:
        std::string msg;
        std::size_t data_offset;
        std::uint64_t size_;


    protected:
        Simpl_cmd(const std::string &cmd, uint64_t cmd_seq, const std::string &data, uint64_t data_offset)
            : data_offset{data_offset} {
            // std::cout << "msg.length()" << msg.length() << "\n";

            size_ = data.length() + data_offset;
            msg = std::string(size_, 0);
            // std::cout << "data.length() + data_offset" << data.length() + data_offset << "\n";
            //
            // std::cout << "msg.length()" << msg.length() << "\n";

            // cmd
            msg.replace(0, cmd.size(), cmd);
            // std::cout << "msg.length()" << msg.length() << "\n";

            // cmd_seq
            uint64_t be_cmd_seq = htobe64(cmd_seq);
            memcpy(msg.data() + cmmn::CMD_SIZE, &be_cmd_seq, sizeof(uint64_t));
            // std::cout << "msg.length()" << msg.length() << "\n";

            // data
            msg.replace(data_offset, data.size(), data);
            // std::cout << "msg.length()" << msg.length() << "\n";

        }

        Simpl_cmd(char *whole, uint64_t size, uint64_t data_offset) : data_offset{data_offset} {
            size_ = size;
            msg = std::string{whole, size_};
        }

    public:

        Simpl_cmd(const std::string &cmd, ssize_t cmd_seq, const std::string &data)
            : data_offset{cmmn::CMD_SIZE + sizeof(uint64_t)} {

            msg = std::string(data.size() + data_offset, 0);

            // cmd
            msg.replace(0, cmd.size(), cmd);
            // cmd_seq
            uint64_t be_cmd_seq = htobe64(cmd_seq);
            memcpy(msg.data() + cmmn::CMD_SIZE, &be_cmd_seq, sizeof(uint64_t));
            // data
            msg.replace(data_offset, data.size(), data);
        }

        Simpl_cmd(char *whole, uint64_t size) : data_offset{cmmn::CMD_SIZE + sizeof(uint64_t)} {
            msg = std::string{whole, size};
        }

        char *raw_msg() {
            return msg.data();
        }

        size_t msg_size() {
            return msg.length();
        }

        std::string get_cmd() {
            return std::string{msg.data(), cmmn::CMD_SIZE};
        }

        std::string get_data() {
            // std::cout << "msg.length() " << msg.length() << "\n";
            // std::cout << "data_offset " << data_offset << "\n";
            //
            // std::cout << "ile " << msg.length() - data_offset << "\n";
            return std::string{msg.data() + data_offset, msg.length() - data_offset};
        }

        uint64_t get_cmd_seq() {
            return be64toh(*(uint64_t *) (msg.data() + cmmn::CMD_SIZE));
        }

    };

    class Cmplx_cmd : public Simpl_cmd {
    public:
        Cmplx_cmd(const std::string &cmd, uint64_t cmd_seq, uint64_t param, const std::string &data)
            : Simpl_cmd(cmd, cmd_seq, data, cmmn::CMD_SIZE + 2 * sizeof(uint64_t)) {
            // param
            uint64_t be_param = htobe64(param);
            memcpy(msg.data() + cmmn::CMD_SIZE + sizeof(uint64_t), &be_param, sizeof(uint64_t));
        }

        Cmplx_cmd(char *whole, uint64_t size) : Simpl_cmd(whole, size, 26) {}

        uint64_t get_param() {
            return be64toh(*(uint64_t *) (msg.data() + cmmn::CMD_SIZE + sizeof(uint64_t)));
        }
    };


    // PLAN TESTÓW - nie harmonogram,
    // tylko jakiego rodzaju tetsy i w jaki sposób należy i chcielisbyśmy wukonać dla naszego projektu
    // Uwaga : nie wymaga żeby tetsy opisane robić, byłoby miło jakby zrobić chociaż część,
    // ale nie jest to wymagane,
    // lepiej żeby plan był pełny i sensowny, a nie wykonany xD
    //
    // nie scenariusze testowe, chodzi o opisanie wszystkich rodzajów tetsów które warto/należoałoby wykonywać dla
    // naszych systemów
    // testy tego przy użyciu tego ( testy cczegoś przy użyciu automatycznych skryptów w bashu/ człowieka / selenium)
    // 1/2/3 strony, wyczerpujące ma być~
    //
    // we wtorek nie będzie grzesia znowu
}

#endif //COMMANDS_HPP
