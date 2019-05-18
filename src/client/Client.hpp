#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../common/common.hpp"
#include "../common/commands.hpp"
#include "../common/exceptions.hpp"
#include "../common/validation.hpp"
#include "../common/sockets.hpp"


#include "Request_parser.hpp"
#include <string>
#include <iostream>
#include <regex>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace reqp = sik_2::request_parser;
namespace excpt = sik_2::exceptions;
namespace valid = sik_2::validation;
namespace cmds = sik_2::commands;
namespace sckt = sik_2::sockets;
namespace cmmn = sik_2::common;

namespace sik_2::client {

    class Client {
    private:
        std::string mcast_addr;
        int32_t cmd_port;
        std::string out_fldr;
        int32_t timeout;

        reqp::Request_parser req_parser;

    public:
        Client() = delete;
        Client(const Client &other) = delete;
        Client(Client &&other) = delete;

        Client(const std::string &mcast_addr, const int32_t cmd_port, const std::string &out_fldr, int32_t timeout) :
            mcast_addr{mcast_addr}, cmd_port{cmd_port}, out_fldr{out_fldr}, timeout{timeout} {
            // if (cmmn::DEBUG) std::cout << "cmd_port " << cmd_port << "\n";

            if (!valid::valid_ip(mcast_addr)) {
                throw excpt::Invalid_param{"mcast_addr = " + mcast_addr};
            }

            if (!valid::in_range_incl<int32_t>(cmd_port, 0, cmmn::MAX_PORT)) {
                throw excpt::Invalid_param{"cmd_port = " + std::to_string(cmd_port)};
            }

            if (!valid::valid_directory(out_fldr)) {
                throw excpt::Invalid_param{"out_fldr = " + out_fldr};
            }
        }

        void run() {
            while (true) {
                std::string param{};
                std::cout << "param :: \"" << param << "\"\n";

                switch (req_parser.next_request(param)) {
                    case reqp::Request::discover: {
                        if (cmmn::DEBUG) std::cout << "DISCOVER" << "\n";
                        send_hello();
                        break;
                    }
                    case reqp::Request::search: {
                        if (cmmn::DEBUG) std::cout << "SEARCH" << "\n";
                        break;
                    }
                    case reqp::Request::fetch: {
                        if (cmmn::DEBUG) std::cout << "FETCH" << "\n";
                        break;
                    }
                    case reqp::Request::upload: {
                        if (cmmn::DEBUG) std::cout << "UPLOAD" << "\n";
                        break;
                    }
                    case reqp::Request::remove: {
                        if (cmmn::DEBUG) std::cout << "REMOVE" << "\n";
                        break;
                    }
                    case reqp::Request::exit: {
                        if (cmmn::DEBUG) std::cout << "EXIT" << "\n";
                        break;
                    }
                    default: {
                        std::cout << "UNKNOWN " << "\n";
                    }
                }
            }
        }

        void send_hello() {
            //W celu poznania listy aktywnych węzłów serwerowych w grupie klient wysyła na adres rozgłoszeniowy
            //MCAST_ADDR i port CMD_PORT pakiet SIMPL_CMD z poleceniem cmd = “HELLO” oraz pustą zawartością pola data.

            sckt::socket_UDP_MCAST sock{mcast_addr, cmd_port, timeout};
            cmds::Simpl_cmd x{cmmn::hello_, 666, "A"};

            int bytes_sent =
                sendto(sock.get_sock(), x.raw_msg(), x.msg_size(), 0,
                       (struct sockaddr *) sock.get_remote_address(), sizeof(*sock.get_remote_address()));
            std::cout << "bytes_sent " << bytes_sent << "\n";


            struct sockaddr_storage sender{};
            ssize_t rcv_len{};

            char buffer[cmmn::MAX_UDP_PACKET_SIZE];

            sock.set_timestamp();
            while (sock.update_timeout()) {
                socklen_t sendsize = sizeof(sender);
                memset(&sender, 0, sizeof(sender));
                rcv_len = recvfrom(sock.get_sock(), buffer, sizeof(buffer), 0, (struct sockaddr *) &sender, &sendsize);

                if (rcv_len < 0) {
                    if (errno != 0) {
                        throw std::system_error(EFAULT, std::generic_category());
                    }
                    break;
                    //syserr("read");
                } else {
                    std::cout << "rcv_len " << rcv_len << "\n";
                    cmmn::print_bytes(rcv_len, buffer);
                }
            }

        }

    };
}

#endif //CLIENT_HPP
