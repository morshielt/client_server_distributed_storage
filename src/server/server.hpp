#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <iostream>
#include <algorithm>
#include <regex>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <csignal>
#include "../common/validation.hpp"
#include "../common/sockets.hpp"
#include "../common/commands.hpp"
#include "../server/file_manager.hpp"

namespace fm = sik_2::file_manager;
namespace valid = sik_2::validation;
namespace sckt = sik_2::sockets;
namespace cmds = sik_2::commands;

namespace sik_2::server {

    class server {

    public:
        server(const std::string &mcast_addr, const int32_t cmd_port, const std::string &shrd_fldr, int64_t
        max_space, int32_t timeout) : mcast_addr{mcast_addr}, cmd_port{cmd_port}, shrd_fldr{shrd_fldr},
                                      timeout{timeout}, f_manager{max_space, shrd_fldr} {

            // IP address notation validation
            valid::validate_ip(mcast_addr);

            // Port range check
            if (!valid::in_range_inclusive<int32_t>(cmd_port, 0, cmmn::MAX_PORT))
                throw excpt::invalid_argument{"cmd_port = " + std::to_string(cmd_port)};
        }

        // Main function
        void run() {
            signal(SIGPIPE, SIG_IGN);
            sckt::socket_UDP_server s{mcast_addr, cmd_port, timeout};
            while (true) {
                get_request(s);
            }
        }

    private:
        void get_request(sckt::socket_UDP_server &s) {
            try {
                struct sockaddr sender{};
                socklen_t sendsize = sizeof(sender);
                memset(&sender, 0, sizeof(sender));
                uint64_t rcv_len{};
                char buffer[cmmn::MAX_UDP_PACKET_SIZE];

                // Get package from socket
                rcv_len = recvfrom(s.get_sock(), buffer, sizeof(buffer), 0, &sender, &sendsize);

                if (rcv_len < cmmn::SIMPL_CMD_SIZE) {
                    return;
                }
                cmds::simpl_cmd cmd{buffer, rcv_len};

                // Recognise by first character and then validate further (cmd & data),
                // then make a new thread to handle valid request
                switch (recognise_request(buffer[0])) {
                    case cmmn::Request::discover: {

                        if (cmd.get_data().empty() && cmd.get_cmd().compare(cmmn::hello_) == 0) {
                            std::thread t{[this, &s, cmd, sender] { ans_discover(s, cmd, sender); }};
                            t.detach();
                        } else
                            invalid_package(sender, "Unknown command.");

                        break;
                    }
                    case cmmn::Request::search: {

                        if (cmd.get_cmd().compare(cmmn::list_) == 0) {
                            std::thread t{[this, &s, cmd, sender] { ans_search(s, cmd, sender); }};
                            t.detach();
                        } else
                            invalid_package(sender, "Unknown command.");

                        break;
                    }
                    case cmmn::Request::fetch: {

                        if (cmd.get_cmd().compare(cmmn::get_) == 0) {
                            std::thread t{[this, &s, cmd, sender] { ans_fetch(s, cmd, sender); }};
                            t.detach();
                        } else
                            invalid_package(sender, "Unknown command.");

                        break;
                    }
                    case cmmn::Request::upload: {

                        cmds::cmplx_cmd c_cmd{buffer, rcv_len};

                        if (c_cmd.get_cmd().compare(cmmn::add_) == 0) {
                            std::thread t{[this, &s, c_cmd, sender] { ans_upload(s, c_cmd, sender); }};
                            t.detach();
                        } else
                            invalid_package(sender, "Unknown command.");

                        break;
                    }
                    case cmmn::Request::remove: {
                        if (cmd.get_cmd().compare(cmmn::del_) == 0 && !cmd.get_data().empty()) {
                            std::thread t{[this, &s, cmd] { ans_remove(s, cmd); }};
                            t.detach();
                        } else
                            invalid_package(sender, "Unknown command.");

                        break;
                    }
                    default: {
                        invalid_package(sender, "Unknown command.");
                    }
                }
            } catch (std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
        }

        void invalid_package(struct sockaddr addr, const std::string &msg) {
            std::string tmp = "[PCKG ERROR] Skipping invalid package from " + cmmn::get_ip(addr)
                + ":" + std::to_string(cmd_port) + ". " + msg + "\n";
            std::cerr << tmp;
        }

        cmmn::Request recognise_request(char x) {

            switch (x) {
                case 'H': { // hello
                    return cmmn::Request::discover;
                }
                case 'L': { // list
                    return cmmn::Request::search;
                }
                case 'G': { // get
                    return cmmn::Request::fetch;
                }
                case 'A': { // add
                    return cmmn::Request::upload;
                }
                case 'D': { // del
                    return cmmn::Request::remove;
                }
                default: {
                    return cmmn::Request::unknown;
                }
            }
        }

        // Send answer to 'HELLO' request
        void ans_discover(sckt::socket_UDP_server &s, cmds::simpl_cmd cmd, struct sockaddr sender) {
            try {
                cmds::cmplx_cmd x{cmmn::good_day_, cmd.get_cmd_seq(), f_manager.get_free_space(), mcast_addr.c_str()};
                sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, &sender, sizeof(sender));

            } catch (std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
        }

        // Send answer to 'LIST' request
        void ans_search(sckt::socket_UDP_server &s, cmds::simpl_cmd cmd, struct sockaddr sender) {
            try {
                // Get list of all available files
                std::string all_files = f_manager.get_files(cmd.get_data());

                // Send file list in few packages if needed
                while (all_files.length() > 1) {
                    std::string tmp = f_manager.cut_nicely(all_files);
                    cmds::simpl_cmd x{cmmn::my_list_, cmd.get_cmd_seq(), tmp};
                    sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, &sender, sizeof(sender));
                }

            } catch (std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
        }

        // Send answer to 'ADD' request
        void ans_upload(sckt::socket_UDP_server &s, cmds::cmplx_cmd cmd, struct sockaddr sender) {
            try {
                // Check whether it's possible to add file
                if (f_manager.add_file(cmd.get_data(), cmd.get_param())) {

                    // It's possible, establish TCP connection & send answer 'CAN ADD'
                    sckt::socket_TCP_server tcp_sock{timeout, cmmn::get_ip(sender)};
                    cmds::cmplx_cmd x{cmmn::can_add_, cmd.get_cmd_seq(), tcp_sock.get_port(), ""};
                    sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0,
                           (struct sockaddr *) &sender, sizeof(sender));

                    f_manager.save_file(tcp_sock, shrd_fldr, cmd.get_data(), cmd.get_param());

                } else {
                    // It's impossible, send answer 'NO WAY'
                    cmds::simpl_cmd x{cmmn::no_way_, cmd.get_cmd_seq(), cmd.get_data()};
                    sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0,
                           (struct sockaddr *) &sender, sizeof(sender));
                }

            } catch (std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
        }

        // Send answer to 'GET' request
        void ans_fetch(sckt::socket_UDP_server &s, cmds::simpl_cmd cmd, struct sockaddr sender) {
            try {
                // Establish TCP connection & send answer 'CONNECT ME', try to send requested file
                sckt::socket_TCP_server tcp_sock{timeout, cmmn::get_ip(sender)};
                cmds::cmplx_cmd x{cmmn::connect_me_, cmd.get_cmd_seq(), tcp_sock.get_port(), cmd.get_data()};

                sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));

                if (!f_manager.send_file(tcp_sock, cmmn::get_path(shrd_fldr, cmd.get_data()))) {
                    invalid_package(sender, "Fetch failed");
                }

            } catch (std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
        }

        // Send answer to 'DEL' request
        void ans_remove(sckt::socket_UDP_server &s, cmds::simpl_cmd cmd) {
            try {
                f_manager.remove_file(cmmn::get_path(shrd_fldr, cmd.get_data()));
            } catch (std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
        }

        std::string mcast_addr;
        int32_t cmd_port;
        std::string shrd_fldr;
        int32_t timeout;
        fm::file_manager f_manager;

    };
}

#endif //SERVER_HPP
