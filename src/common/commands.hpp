#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include <cstdint>
#include <array>
#include <cstring>
#include "common.hpp"

namespace cmmn = sik_2::common;

namespace sik_2::commands {

    // TODO struct __packed__ blablablablablablablbablalbablalb
    class Simpl_cmd {

    protected:
        std::string msg;
        std::size_t data_offset;
        std::uint64_t size_;

    protected:
        Simpl_cmd(const std::string &cmd, uint64_t cmd_seq, const std::string &data, uint64_t data_offset)
            : data_offset{data_offset}, size_{data.length() + data_offset} {

            // cmd
            msg = std::string(size_, 0);
            msg.replace(0, cmd.size(), cmd);

            // cmd_seq
            uint64_t be_cmd_seq = htobe64(cmd_seq);
            memcpy(msg.data() + cmmn::CMD_SIZE, &be_cmd_seq, sizeof(uint64_t));

            // data
            msg.replace(data_offset, data.size(), data);
            std::cout << ">> 1 LENGTH (get_msg_size): " << this->get_msg_size() << "\n";
            if (cmmn::DEBUG) print_bytes();
        }

        Simpl_cmd(char *whole, uint64_t size, uint64_t data_offset)
            : data_offset{data_offset}, size_{size} {

            msg = std::string{whole, size_};
            std::cout << ">> 2 LENGTH (get_msg_size): " << this->get_msg_size() << "\n";
            if (cmmn::DEBUG) print_bytes();
        }

    public:
        Simpl_cmd(const std::string &cmd, uint64_t cmd_seq, const std::string &data)
            : Simpl_cmd(cmd, cmd_seq, data, cmmn::CMD_SIZE + sizeof(uint64_t)) {

            std::cout << ">> 3 LENGTH (get_msg_size): " << this->get_msg_size() << "\n";
        }

        Simpl_cmd(char *whole, uint64_t size)
            : Simpl_cmd(whole, size, cmmn::CMD_SIZE + sizeof(uint64_t)) {

            std::cout << ">> 4 LENGTH (get_msg_size): " << this->get_msg_size() << "\n";
        }

        char *get_raw_msg() {
            return msg.data();
        }

        size_t get_msg_size() {
            return msg.length();
        }

        std::string get_cmd() {
            return std::string{msg.data(), cmmn::CMD_SIZE};
        }

        std::string get_data() {
            return std::string{msg.data() + data_offset, msg.length() - data_offset};
        }

        uint64_t get_cmd_seq() {
            return be64toh(*(uint64_t *) (msg.data() + cmmn::CMD_SIZE));
        }

        size_t get_data_offset() {
            return data_offset;
        }

        void print_bytes() {
            for (int j = 0; j < cmmn::CMD_SIZE; ++j) {
                printf("%c ", (const unsigned char) msg.data()[j]);
            }
            std::cout << " | " << be64toh(*(uint64_t *) (msg.data() + cmmn::CMD_SIZE)) << " | ";

            if (data_offset == cmmn::CMD_SIZE + 2 * sizeof(uint64_t)) {
                std::cout << be64toh(*(uint64_t *) (msg.data() + cmmn::CMD_SIZE + sizeof(uint64_t))) << " | ";
            }

            for (int i = data_offset; i < msg.length(); ++i) {
                printf("%c ", msg.data()[i]);
            }
            std::cout << " |\n";
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

        Cmplx_cmd(char *whole, uint64_t size)
            : Simpl_cmd(whole, size, cmmn::CMD_SIZE + 2 * sizeof(uint64_t)) {}

        uint64_t get_param() {
            return be64toh(*(uint64_t *) (msg.data() + cmmn::CMD_SIZE + sizeof(uint64_t)));
        }
    };
}

#endif //COMMANDS_HPP
