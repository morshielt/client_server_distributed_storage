#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <iostream>
#include <algorithm>
#include <regex>
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
        std::string shdr_fldr;
        int32_t timeout;
        fm::file_manager f_manager;

    public:
        server(const std::string &mcast_addr, const int32_t cmd_port, const std::string &shrd_fldr, int64_t
        max_space, int32_t timeout) : mcast_addr{mcast_addr}, cmd_port{cmd_port}, shdr_fldr{shrd_fldr},
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
            while (true)
                get_request();
        }

    private:
        void get_request() {
            //TODO Serwer powinien podłączyć się do grupy rozgłaszania ukierunkowanego
            // pod wskazanym adresem MCAST_ADDR. Serwer powinien nasłuchiwać na porcie
            // CMD_PORT poleceń otrzymanych z sieci protokołem UDP {{także na swoim adresie
            // unicast}}?? Serwer powinien reagować na pakiety UDP zgodnie z protokołem opisanym wcześniej.
            struct sockaddr sender{};
            socklen_t sendsize = sizeof(sender);
            memset(&sender, 0, sizeof(sender));
            uint64_t rcv_len{};
            char buffer[cmmn::MAX_UDP_PACKET_SIZE];

            sckt::socket_UDP s{mcast_addr, cmd_port, timeout};

            rcv_len = recvfrom(s.get_sock(), buffer, sizeof(buffer), 0, &sender, &sendsize);
            s.set_sender(sender);

            if (rcv_len < 0) {
                // TODO EWOULDBLOCK/EAGAIN???
                throw std::system_error(EFAULT, std::generic_category());
                // syserr("read");
            }

            switch (recognise_request(buffer[0])) {
                case cmmn::Request::discover: {
                    if (cmmn::DEBUG) std::cout << "DISCOVER" << "\n";
                    cmds::simpl_cmd cmd{buffer, rcv_len};

                    if (cmd.get_data().empty() && cmd.get_cmd().compare(cmmn::hello_) == 0)
                        ans_discover(s, cmd);
                    else
                        invalid_package(sender, cmd_port, "Unknown command.");

                    break;
                }
                case cmmn::Request::search: {
                    if (cmmn::DEBUG) std::cout << "SEARCH" << "\n";
                    cmds::simpl_cmd cmd{buffer, rcv_len};

                    if (cmd.get_cmd().compare(cmmn::list_) == 0)
                        ans_search(s, cmd);
                    else
                        invalid_package(sender, cmd_port, "Unknown command.");

                    break;
                }
                case cmmn::Request::fetch: {
                    if (cmmn::DEBUG) std::cout << "FETCH" << "\n";
                    break;
                }
                case cmmn::Request::upload: {
                    if (cmmn::DEBUG) std::cout << "UPLOAD" << "\n";
                    cmds::cmplx_cmd cmd{buffer, rcv_len};

                    if (cmd.get_cmd().compare(cmmn::add_) == 0)
                        ans_upload(s, cmd);
                    else
                        invalid_package(sender, cmd_port, "Unknown command.");

                    break;
                }
                case cmmn::Request::remove: {
                    if (cmmn::DEBUG) std::cout << "REMOVE" << "\n";
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
                std::string tmp = cut_nicely(all_files);
                std::cout << "all after : \"" << all_files << "\"\n";
                std::cout << "tmp :  \"" << tmp << "\"\n";

                cmds::simpl_cmd x{cmmn::my_list_, cmd.get_cmd_seq(), tmp};
                std::cout << "send bytes:::::: " <<
                          sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, &sender, sizeof(sender));
                std::cout << "sent my_list\n";
            } while (all_files.length() > 0);

        }

        std::string cut_nicely(std::string &str) {
            std::string tmp{str, 0, std::min<size_t>(cmmn::MAX_UDP_PACKET_SIZE, str.length())};

            size_t last = tmp.find_last_of('\n');
            std::cout << "last " << last << "end: " << (tmp.length() - 1) << "\n";

            tmp = std::string{tmp, 0, last};
            str = std::string{str, last + 1, str.length()};

            return tmp;
        }

        void ans_upload(sckt::socket_UDP &s, cmds::cmplx_cmd cmd) {

            struct sockaddr sender = s.get_sender();

            // TODO lock
            if (!cmd.get_data().empty() && cmd.get_data().find('/') == std::string::npos &&
                cmd.get_param() < f_manager.get_free_space() && f_manager.filename_nontaken(cmd.get_data())) {

                sckt::socket_TCP_in tcp_sock{timeout, cmmn::get_ip(sender)};

                cmds::cmplx_cmd x{cmmn::can_add_, cmd.get_cmd_seq(), tcp_sock.get_port(), ""};

                sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));

                tcp_sock.get_connection();
                f_manager.save_file(tcp_sock.get_sock(), shdr_fldr + cmd.get_data(), cmd.get_param());

            } else {
                cmds::simpl_cmd x{cmmn::no_way_, cmd.get_cmd_seq(), cmd.get_data()};
                sendto(s.get_sock(), x.get_raw_msg(), x.get_msg_size(), 0, (struct sockaddr *) &sender, sizeof(sender));
            }
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
