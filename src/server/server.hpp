#ifndef SERVER_HPP
#define SERVER_HPP
// TODO ZAMIAST THROW CERR
#include <string>
#include <iostream>
#include <algorithm>
#include <regex>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/sendfile.h>
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

            valid::validate_ip(mcast_addr);

            if (!valid::in_range_inclusive<int32_t>(cmd_port, 0, cmmn::MAX_PORT))
                throw excpt::invalid_argument{"cmd_port = " + std::to_string(cmd_port)};
        }

        void run() {
            sckt::socket_UDP_server s{mcast_addr, cmd_port, timeout};
            while (true) {
                get_request(s);
            }
        }

    private:
        void get_request(sckt::socket_UDP_server &s) {
            struct sockaddr sender{};
            socklen_t sendsize = sizeof(sender);
            memset(&sender, 0, sizeof(sender));
            uint64_t rcv_len{};
            char buffer[cmmn::MAX_UDP_PACKET_SIZE];

            rcv_len = recvfrom(s.get_sock(), buffer, sizeof(buffer), 0, &sender, &sendsize);

            if (rcv_len < 0) {
                // TODO EWOULDBLOCK/EAGAIN???
                throw std::system_error(EFAULT, std::generic_category());
            }
            cmds::simpl_cmd cmd{buffer, rcv_len};

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
                        std::thread t{[this, &s, c_cmd, sender] {
                            ans_upload(s, c_cmd, sender);
                        }};
                        t.detach();
                    } else
                        invalid_package(sender, "Unknown command.");
                    break;
                }
                case cmmn::Request::remove: {
                    if (cmmn::DEBUG) std::cout << "REMOVE" << "\n";

                    std::thread t{[this, &s, cmd] { ans_remove(s, cmd); }};
                    t.detach();
                    break;
                }
                default: {
                    if (cmmn::DEBUG) std::cout << "UNKNOWN " << "\n";
                    invalid_package(sender, "Unknown command.");
                }
            }
        }

        void invalid_package(struct sockaddr addr, const std::string &msg) {
            std::cout << "[PCKG ERROR] Skipping invalid package from " << cmmn::get_ip(addr)
                      << ":" << cmd_port << ". " + msg + "\n";
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

        void ans_discover(sckt::socket_UDP_server &s, cmds::simpl_cmd cmd, struct sockaddr sender) {

            cmds::cmplx_cmd x{cmmn::good_day_, cmd.get_cmd_seq(), f_manager.get_free_space(), mcast_addr.c_str()};

            sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, &sender, sizeof(sender));
            std::cout << "sent good_day\n";
        }

        void ans_search(sckt::socket_UDP_server &s, cmds::simpl_cmd cmd, struct sockaddr sender) {
            std::string all_files = f_manager.get_files(cmd.get_data());

            while (all_files.length() > 1) {
                std::string tmp = f_manager.cut_nicely(all_files);
                std::cout << "tmp :: \"" << tmp << "\"\n";
                cmds::simpl_cmd x{cmmn::my_list_, cmd.get_cmd_seq(), tmp};
                sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, &sender, sizeof(sender));
            }
        }

        void ans_upload(sckt::socket_UDP_server &s, cmds::cmplx_cmd cmd, struct sockaddr sender) {
            if (f_manager.add_file(cmd.get_data(), cmd.get_param())) {

                sckt::socket_TCP_server tcp_sock{timeout, cmmn::get_ip(sender)};
                cmds::cmplx_cmd x{cmmn::can_add_, cmd.get_cmd_seq(), tcp_sock.get_port(), ""};
                sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));

                f_manager.save_file(tcp_sock, shrd_fldr, cmd.get_data(), cmd.get_param());

            } else {
                cmds::simpl_cmd x{cmmn::no_way_, cmd.get_cmd_seq(), cmd.get_data()};
                sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));
            }
        }

        void ans_fetch(sckt::socket_UDP_server &s, cmds::simpl_cmd cmd, struct sockaddr sender) {

            sckt::socket_TCP_server tcp_sock{timeout, cmmn::get_ip(sender)};
            cmds::cmplx_cmd x{cmmn::connect_me_, cmd.get_cmd_seq(), tcp_sock.get_port(), cmd.get_data()};

            sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));

            if (!f_manager.send_file(tcp_sock, cmmn::get_path(shrd_fldr, cmd.get_data()))) {
                invalid_package(sender, "File requested in fetch doesn't exist.");
            }
        }

        void ans_remove(sckt::socket_UDP_server &s, cmds::simpl_cmd cmd) {
            f_manager.remove_file(cmmn::get_path(shrd_fldr, cmd.get_data()));
        }

        std::string mcast_addr;
        int32_t cmd_port;
        std::string shrd_fldr;
        int32_t timeout;
        fm::file_manager f_manager;
    };
}

#endif //SERVER_HPP
