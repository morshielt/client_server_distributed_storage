#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <iostream>
#include <regex>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../common/validation.hpp"
#include "../common/sockets.hpp"
#include "../common/commands.hpp"

namespace valid = sik_2::validation;
namespace sckt = sik_2::sockets;
namespace cmds = sik_2::commands;

namespace sik_2::server {

    class Server {
    private:
        std::string mcast_addr;
        int32_t cmd_port;
        std::string shdr_fldr;
        int32_t timeout;

    public:
        Server(const std::string &mcast_addr, const int32_t cmd_port, const std::string &shdr_fldr, int32_t timeout) :
            mcast_addr{mcast_addr}, cmd_port{cmd_port}, shdr_fldr{shdr_fldr}, timeout{timeout} {
            // if (cmmn::DEBUG) std::cout << "cmd_port " << cmd_port << "\n";

            if (!valid::valid_ip(mcast_addr)) {
                throw excpt::Invalid_param{"mcast_addr = " + mcast_addr};
            }

            if (!valid::in_range_incl<int32_t>(cmd_port, 0, cmmn::MAX_PORT)) {
                throw excpt::Invalid_param{"cmd_port = " + std::to_string(cmd_port)};
            }

            if (!valid::valid_directory(shdr_fldr)) {
                throw excpt::Invalid_param{"out_fldr = " + shdr_fldr};
            }
        }

        void run() {
            // while (true)
            get_request();
        }

        void get_request() {
            struct sockaddr_storage sender{};
            ssize_t rcv_len{};

            sckt::socket_UDP s{mcast_addr, cmd_port, timeout};
            sckt::socket_UDP s2{mcast_addr, cmd_port, timeout};

            char buffer[cmmn::MAX_UDP_PACKET_SIZE];

            while (true) {
                socklen_t sendsize = sizeof(sender);
                memset(&sender, 0, sizeof(sender));

                rcv_len = recvfrom(s.get_sock(), buffer, sizeof(buffer), 0, (struct sockaddr *) &sender, &sendsize);

                if (rcv_len < 0) {
                    throw std::system_error(EFAULT, std::generic_category());
                    //syserr("read");
                } else {
                    cmmn::print_bytes(rcv_len, buffer);
                }

                cmds::Cmplx_cmd x{cmmn::good_day_, 666, 0, mcast_addr.c_str(), mcast_addr.length()};

                // odsyłamy czas tam, skąd dostaliśmy requesta
                sendto(s.get_sock(), x.raw_msg(), x.msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));
                std::cout << "sent answer\n";
            }
        }

    };
}

#endif //SERVER_HPP
