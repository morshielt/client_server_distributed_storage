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

    private:
        std::string mcast_addr;
        int32_t cmd_port;
        std::string shrd_fldr;
        int32_t timeout;
        fm::file_manager f_manager;

    public:
        server(const std::string &mcast_addr, const int32_t cmd_port, const std::string &shrd_fldr, int64_t
        max_space, int32_t timeout) : mcast_addr{mcast_addr}, cmd_port{cmd_port}, shrd_fldr{shrd_fldr},
                                      timeout{timeout}, f_manager{max_space, shrd_fldr} {

            if (!valid::valid_ip(mcast_addr)) {
                throw excpt::invalid_argument{"mcast_addr = " + mcast_addr};
            }

            if (!valid::in_range_incl<int32_t>(cmd_port, 0, cmmn::MAX_PORT)) {
                throw excpt::invalid_argument{"cmd_port = " + std::to_string(cmd_port)};
            }

            //TODO tu wywalać timeout czy mogę dopiero w sockecie?
        }

        void run() {
            sckt::socket_UDP s{mcast_addr, cmd_port, timeout};
            while (true) {
                std::cout << "jestem promptem~\n";
                get_request(s);
            }
        }

    private:
        void get_request(sckt::socket_UDP &s) {
            //TODO Serwer powinien podłączyć się do grupy rozgłaszania ukierunkowanego
            // pod wskazanym adresem MCAST_ADDR. Serwer powinien nasłuchiwać na porcie
            // CMD_PORT poleceń otrzymanych z sieci protokołem UDP {{także na swoim adresie
            // unicast}}?? Serwer powinien reagować na pakiety UDP zgodnie z protokołem opisanym wcześniej.
            struct sockaddr sender{};
            socklen_t sendsize = sizeof(sender);
            memset(&sender, 0, sizeof(sender));
            uint64_t rcv_len{};
            char buffer[cmmn::MAX_UDP_PACKET_SIZE];

            // sckt::socket_UDP s{mcast_addr, cmd_port, timeout};

            rcv_len = recvfrom(s.get_sock(), buffer, sizeof(buffer), 0, &sender, &sendsize);
            s.set_sender(sender);

            if (rcv_len < 0) {
                // TODO EWOULDBLOCK/EAGAIN???
                throw std::system_error(EFAULT, std::generic_category());
                // syserr("read");
            }
            cmds::simpl_cmd cmd{buffer, rcv_len};

            switch (recognise_request(buffer[0])) {
                case cmmn::Request::discover: {
                    if (cmmn::DEBUG) std::cout << "DISCOVER" << "\n";
                    // cmds::simpl_cmd cmd{buffer, rcv_len};

                    if (cmd.get_data().empty() && cmd.get_cmd().compare(cmmn::hello_) == 0) {
                        std::thread t{[this, &s, cmd] { ans_discover(s, cmd); }};
                        t.detach();
                    } else
                        invalid_package(sender, cmd_port, "Unknown command.");

                    break;
                }
                case cmmn::Request::search: {
                    if (cmmn::DEBUG) std::cout << "SEARCH" << "\n";
                    // cmds::simpl_cmd cmd{buffer, rcv_len};

                    if (cmd.get_cmd().compare(cmmn::list_) == 0) {
                        std::thread t{[this, &s, cmd] { ans_search(s, cmd); }};
                        t.detach();
                    } else
                        invalid_package(sender, cmd_port, "Unknown command.");

                    break;
                }
                case cmmn::Request::fetch: {
                    if (cmmn::DEBUG) std::cout << "FETCH" << "\n";
                    // cmds::simpl_cmd cmd{buffer, rcv_len};
                    if (cmd.get_cmd().compare(cmmn::get_) == 0 && !f_manager.filename_nontaken(cmd.get_data())) {
                        std::thread t{[this, &s, cmd] { ans_fetch(s, cmd); }};
                        t.detach();
                    } else
                        invalid_package(sender, cmd_port, "Unknown command.");
                    break;
                }
                case cmmn::Request::upload: {
                    if (cmmn::DEBUG) std::cout << "UPLOAD" << "\n";
                    cmds::cmplx_cmd c_cmd{buffer, rcv_len};

                    if (c_cmd.get_cmd().compare(cmmn::add_) == 0) {
                        std::thread t{[this, &s, c_cmd] {
                            ans_upload(s, c_cmd);
                        }};
                        t.detach();
                    } else
                        invalid_package(sender, cmd_port, "Unknown command.");
                    break;
                }
                case cmmn::Request::remove: {
                    if (cmmn::DEBUG) std::cout << "REMOVE" << "\n";
                    // cmds::simpl_cmd cmd{buffer, rcv_len};

                    std::thread t{[this, &s, cmd] { ans_remove(s, cmd); }};
                    t.detach();
                    break;
                }
                default: {
                    if (cmmn::DEBUG) std::cout << "UNKNOWN " << "\n";
                    invalid_package(sender, cmd_port, "Unknown command.");
                }
            }
        }

        void invalid_package(struct sockaddr addr, int32_t port, const std::string &msg) {
            std::cout << "[PCKG ERROR] Skipping invalid package from " << cmmn::get_ip(addr)
                      << ":" << port << ". " + msg + "\n";
        }

        void ans_discover(sckt::socket_UDP &s, cmds::simpl_cmd cmd) {

            cmds::cmplx_cmd x{cmmn::good_day_, cmd.get_cmd_seq(), f_manager.get_free_space(), mcast_addr.c_str()};

            struct sockaddr sender = s.get_sender();
            sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, &sender, sizeof(sender));
            std::cout << "sent good_day\n";
        }

        void ans_search(sckt::socket_UDP &s, cmds::simpl_cmd cmd) {

            struct sockaddr sender = s.get_sender();

            std::string all_files = f_manager.get_files(cmd.get_data());
            std::cout << "all : \"" << all_files << "\"\n";

            do {
                std::cout << "all : \"" << all_files << "\"\n";
                std::string tmp = f_manager.cut_nicely(all_files);
                std::cout << "all after : \"" << all_files << "\"\n";
                std::cout << "tmp :  \"" << tmp << "\"\n";

                cmds::simpl_cmd x{cmmn::my_list_, cmd.get_cmd_seq(), tmp};
                std::cout << "send bytes:::::: " <<
                          sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, &sender, sizeof(sender));
                std::cout << "sent my_list\n";
            } while (all_files.length() > 0);
        }

        void ans_upload(sckt::socket_UDP &s, cmds::cmplx_cmd cmd) {

            struct sockaddr sender = s.get_sender();

            // TODO lock
            if (f_manager.add_file(cmd.get_data(), cmd.get_param())) {

                sckt::socket_TCP_server tcp_sock{timeout, cmmn::get_ip(sender)};

                cmds::cmplx_cmd x{cmmn::can_add_, cmd.get_cmd_seq(), tcp_sock.get_port(), ""};

                sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));

                tcp_sock.get_connection();
                f_manager.save_file(tcp_sock.get_sock(), cmmn::get_path(shrd_fldr, cmd.get_data()), cmd.get_param());

            } else {
                cmds::simpl_cmd x{cmmn::no_way_, cmd.get_cmd_seq(), cmd.get_data()};
                sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));
            }
        }

        void ans_fetch(sckt::socket_UDP &s, cmds::simpl_cmd cmd) {

            struct sockaddr sender = s.get_sender();

            sckt::socket_TCP_server tcp_sock{timeout, cmmn::get_ip(sender)};

            cmds::cmplx_cmd x{cmmn::connect_me_, cmd.get_cmd_seq(), tcp_sock.get_port(), cmd.get_data()};

            sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));

            tcp_sock.get_connection();
            f_manager.send_file(cmmn::get_path(shrd_fldr, cmd.get_data()), tcp_sock.get_sock());
        }

        void ans_remove(sckt::socket_UDP &s, cmds::simpl_cmd cmd) {
            // TODO lock
            f_manager.remove_file(cmmn::get_path(shrd_fldr, cmd.get_data()));
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
    };
}

#endif //SERVER_HPP
