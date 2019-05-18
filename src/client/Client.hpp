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

namespace excpt = sik_2::exceptions;
namespace valid = sik_2::validation;
namespace cmds = sik_2::commands;
namespace sckt = sik_2::sockets;

namespace sik_2::client {

    class Client {
    private:
        std::string mcast_addr;
        int32_t cmd_port;
        std::string out_fldr;
        int32_t timeout;

        sik_2::client::Request_parser req_parser;

    public:
        Client() = delete;
        Client(const Client &other) = delete;
        Client(Client &&other) = delete;

        Client(std::string mcast_addr, const int32_t cmd_port, std::string out_fldr, int32_t timeout) :
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
        };


        void run() {
            // while (true) {
            //     req_parser.next_request();
            // }
            // send_hello();
            send_hello();

        }

        void send_hello() {
            //W celu poznania listy aktywnych węzłów serwerowych w grupie klient wysyła na adres rozgłoszeniowy
            //MCAST_ADDR i port CMD_PORT pakiet SIMPL_CMD z poleceniem cmd = “HELLO” oraz pustą zawartością pola data.
            // int sock, optval;
            struct sockaddr_in remote_address;

            /* ustawienie adresu i portu odbiorcy */
            remote_address.sin_family = AF_INET;
            remote_address.sin_port = htons(cmd_port);

            if (inet_aton(mcast_addr.c_str(), &remote_address.sin_addr) == 0)
                cmmn::syserr("inet_aton", __LINE__, __FILE__);


            sckt::socket_UPD_MCAST sock{mcast_addr, cmd_port};
            // sckt::socket_UPD_MCAST socket_upd_mcast1{mcast_addr, cmd_port};

            cmds::Simpl_cmd x{cmmn::hello_, 666, "", 0};

            int bytes_sent =
                sendto(sock.get_sock(), x.raw_msg(), x.msg_size(), 0,
                       (struct sockaddr *) sock.get_remote_address(),
                       sizeof(*sock.get_remote_address()));
            std::cout << "bytes_sent " << bytes_sent << "\n";
        }

    };
}

#endif //CLIENT_HPP
