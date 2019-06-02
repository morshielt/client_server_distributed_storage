#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include <cstdint>
#include <array>
#include <cstring>
#include "common.hpp"

namespace cmmn = sik_2::common;

namespace sik_2::commands {

    class simpl_cmd {

    public:
        simpl_cmd(const std::string &cmd, uint64_t cmd_seq, const std::string &data)
            : simpl_cmd(cmd, cmd_seq, data, cmmn::CMD_SIZE + sizeof(uint64_t)) {}

        simpl_cmd(char *whole, uint64_t size)
            : simpl_cmd(whole, size, cmmn::CMD_SIZE + sizeof(uint64_t)) {}

        size_t get_msg_size() const {
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

        char *get_raw_msg() const {
            return const_cast<char *>(msg.data());
        }

    protected:
        simpl_cmd(const std::string &cmd, uint64_t cmd_seq, const std::string &data, uint64_t data_offset)
            : data_offset{data_offset}, size_{data.length() + data_offset} {

            // cmd
            msg = std::string(size_, 0);
            msg.replace(0, cmd.size(), cmd);

            // cmd_seq
            uint64_t be_cmd_seq = htobe64(cmd_seq);
            memcpy(msg.data() + cmmn::CMD_SIZE, &be_cmd_seq, sizeof(uint64_t));

            // data
            msg.replace(data_offset, data.size(), data);
        }

        simpl_cmd(char *whole, uint64_t size, uint64_t data_offset)
            : data_offset{data_offset}, size_{size} {

            msg = std::string{whole, size_};
        }

        std::string msg;
        std::size_t data_offset;
        std::uint64_t size_;
    };

    class cmplx_cmd : public simpl_cmd {

    public:
        cmplx_cmd(const std::string &cmd, uint64_t cmd_seq, uint64_t param, const std::string &data)
            : simpl_cmd(cmd, cmd_seq, data, cmmn::CMD_SIZE + 2 * sizeof(uint64_t)) {

            // param
            uint64_t be_param = htobe64(param);
            memcpy(msg.data() + cmmn::CMD_SIZE + sizeof(uint64_t), &be_param, sizeof(uint64_t));
        }

        cmplx_cmd(char *whole, uint64_t size)
            : simpl_cmd(whole, size, cmmn::CMD_SIZE + 2 * sizeof(uint64_t)) {}

        uint64_t get_param() const {
            return be64toh(*(uint64_t *) (msg.data() + cmmn::CMD_SIZE + sizeof(uint64_t)));
        }
    };
}

#endif //COMMANDS_HPP
