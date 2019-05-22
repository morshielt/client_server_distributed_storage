#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <iostream>
#include <regex>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "../common/common.hpp"
#include "../common/commands.hpp"
#include "../common/exceptions.hpp"
#include "../common/validation.hpp"
#include "../common/sockets.hpp"
#include "request_parser.hpp"

namespace fs =  boost::filesystem;
namespace reqp = sik_2::request_parser;
namespace excpt = sik_2::exceptions;
namespace valid = sik_2::validation;
namespace cmds = sik_2::commands;
namespace sckt = sik_2::sockets;
namespace cmmn = sik_2::common;


// TODO nowe sockety dla fetcha
namespace sik_2::client {

    class client {

    private:
        std::string mcast_addr;
        int32_t cmd_port;
        std::string out_fldr;
        int32_t timeout;
        uint64_t seq{};

        std::set<std::pair<uint64_t, std::string>, std::greater<>> servers{};
        struct sockaddr sender{};

        std::map<std::string, std::string> available_files{}; // filename -> ip

        reqp::request_parser req_parser;

        uint64_t get_next_seq() {
            return seq++;
        }

    public:
        client() = delete;
        client(const client &other) = delete;
        client(client &&other) = delete;

        client(const std::string &mcast_addr, const int32_t cmd_port, const std::string &out_fldr, int32_t timeout)
            : mcast_addr{mcast_addr}, cmd_port{cmd_port}, out_fldr{out_fldr}, timeout{timeout} {

            if (!valid::valid_ip(mcast_addr))
                throw excpt::invalid_argument{"mcast_addr = " + mcast_addr};

            if (!valid::in_range_incl<int32_t>(cmd_port, 0, cmmn::MAX_PORT))
                throw excpt::invalid_argument{"cmd_port = " + std::to_string(cmd_port)};

            if (!valid::valid_directory(out_fldr))
                throw excpt::invalid_argument{"out_fldr = " + out_fldr};
        }

        void run() {

            std::string param{};

            while (true) {
                std::cout << "param :: \"" << param << "\"\n";

                switch (req_parser.next_request(param)) {
                    case cmmn::Request::discover: {
                        if (cmmn::DEBUG) std::cout << "DISCOVER" << "\n";
                        do_discover(false);
                        break;
                    }
                    case cmmn::Request::search: {
                        if (cmmn::DEBUG) std::cout << "SEARCH" << "\n";
                        do_search(param);
                        break;
                    }
                    case cmmn::Request::fetch: {
                        if (cmmn::DEBUG) std::cout << "FETCH" << "\n";
                        break;
                    }
                    case cmmn::Request::upload: {
                        if (cmmn::DEBUG) std::cout << "UPLOAD" << "\n";
                        do_upload(param);
                        break;
                    }
                    case cmmn::Request::remove: {
                        if (cmmn::DEBUG) std::cout << "REMOVE" << "\n";
                        break;
                    }
                    case cmmn::Request::exit: {
                        if (cmmn::DEBUG) std::cout << "EXIT" << "\n";
                        break;
                    }
                    default: {
                        std::cout << "UNKNOWN " << "\n";
                    }
                }
            }
        }

    private:
        template<typename Send_type, typename Recv_type>
        void send_and_recv(Send_type s_cmd, const std::string &addr, uint64_t cmd_seq,
                           std::function<bool(Recv_type ans)> &func) {

            char buffer[cmmn::MAX_UDP_PACKET_SIZE];
            ssize_t ret{0};

            sckt::socket_UDP_MCAST udpm_sock{addr, cmd_port, timeout};

            // send request
            ret = sendto(udpm_sock.get_sock(), s_cmd.get_raw_msg(), s_cmd.get_msg_size(), 0, (struct sockaddr *)
                udpm_sock.get_remote_address(), sizeof(*udpm_sock.get_remote_address()));

            // TODO czy my w ogóle chcemy ifować EWOULDBLOCK i EAGAIN?
            // co to ma być? D:
            if (ret == -1 && !(errno == EWOULDBLOCK || errno == EAGAIN)) {
                std::cout << __LINE__ << " " << __FILE__ << "\n";
                throw excpt::socket_excpt(std::strerror(errno));
            }

            // wait 'timeout' seconds for answers
            udpm_sock.set_timestamp();
            bool teges;
            while (teges = udpm_sock.update_timeout()) {
                assert(teges == true);
                socklen_t sendsize = sizeof(sender);
                memset(&sender, 0, sizeof(sender));
                ret = recvfrom(udpm_sock.get_sock(), buffer, sizeof(buffer), 0, &sender, &sendsize);

                if (ret == -1) {
                    // TODO czy my w ogóle chcemy ifować te rzeczy?
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        // TODO czemu tu jest break? :OOOOOO
                        break;
                    }
                    std::cout << __LINE__ << " " << __FILE__ << "\n";
                    throw excpt::socket_excpt(std::strerror(errno));
                }

                std::cout << "receive bytes :::::::::: " << ret << "\n";

                // check cmd & cmd_seq
                Recv_type y{buffer, static_cast<uint64_t>(ret)};
                // y.print_bytes();
                // validate cmd_seq & lambda result
                std::cout << "cmd seqsy : " << y.get_cmd_seq() << " : " << cmd_seq << "\n";
                if (y.get_cmd_seq() != cmd_seq || !func(y)) {
                    std::cout << "[PCKG ERROR] Skipping invalid package from "
                              << cmmn::get_ip(sender) << ":" << cmd_port << "\n";
                }
            }
            // assert(teges == false); no tak to nigdy nie będzie przez tego break'a

        }

        void do_discover(bool upload) {

            auto lambd = std::function([this, upload](cmds::cmplx_cmd ans)->bool {
                // validate cmd - we expect "GOOD DAY"
                if (ans.get_cmd().compare(cmmn::good_day_) != 0) {
                    return false;
                }

                if (!upload) {
                    std::cout << "Found " << cmmn::get_ip(sender) << " (" << ans.get_data() << ") with free space "
                              << ans.get_param() << "\n";
                } else {
                    servers.insert({ans.get_param(), cmmn::get_ip(sender)});
                }
                return true;
            });

            if (upload) servers.clear();
            send_and_recv(cmds::simpl_cmd{cmmn::hello_, get_next_seq(), ""}, mcast_addr, seq, lambd);
        }

        // TODO nowy thread na do_shit~
        // TODO nie działa.
        void do_upload(const std::string &filename) {

            if (!fs::exists(cmmn::get_path(out_fldr, filename))) {
                std::cerr << "File " << filename << " does not exist\n";
                return;
            }

            do_discover(true);

            uint64_t file_size = fs::file_size(cmmn::get_path(out_fldr, filename));

            if (file_size > servers.begin()->first) {
                std::cerr << "File " << filename << " too big\n";
                return;
            }

            bool accept = false;

            if (!servers.empty()) {
                for (auto &ser : servers) {

                    auto lambd = std::function([this, &accept, filename, file_size](cmds::simpl_cmd ans)->bool {
                        // validate answers - we expect "NO_WAY" or "CAN_ADD"
                        if (ans.get_cmd().compare(cmmn::no_way_) == 0) {

                            return (ans.get_data().compare(filename) == 0);

                        } else if (ans.get_cmd().compare(cmmn::can_add_) == 0) {
                            accept = true;

                            cmds::cmplx_cmd cst{ans.get_raw_msg(), ans.get_msg_size()};

                            sckt::socket_TCP_out tcp_sock{timeout, cmmn::get_ip(sender), (int32_t) cst.get_param()};

                            try {
                                cmmn::send_file(cmmn::get_path(out_fldr, filename), file_size, tcp_sock.get_sock());
                                std::cout << "File " << filename << " uploaded ("
                                          << cmmn::get_ip(sender) << ":" << (int32_t) cst.get_param()
                                          << ")\n";
                            }
                            catch (excpt::file_excpt &e) {
                                std::cout << "File " << filename << " uploading failed ("
                                          << cmmn::get_ip(sender) << ":" << (int32_t) cst.get_param()
                                          << ") " << e.what() << "\n";
                            }


                            return true;
                        } else { // unknown cmd
                            return false;
                        }
                    });

                    // while nobody has accepted our "ADD" request, we keep trying
                    if (!accept) {
                        std::cout << "filesize :: " << file_size << "\n";
                        send_and_recv(cmds::cmplx_cmd{cmmn::add_, get_next_seq(), file_size, filename},
                                      ser.second, seq, lambd);
                    } else {
                        break;
                    }
                }
            }
        }

        void do_search(std::string sub) {
            // search %s – klient powinien uznać polecenie za prawidłowe, także jeśli
            // podany ciąg znaków %s jest pusty. Po otrzymaniu tego polecenia klient
            // wysyła po sieci do węzłów serwerowych zapytanie w celu wyszukania plików
            // zawierających ciąg znaków podany przez użytkownika (lub wszystkich plików
            // jeśli ciąg znaków %s jest pusty), a następnie przez TIMEOUT sekund nasłuchuje
            // odpowiedzi od węzłów serwerowych. Otrzymane listy plików powinny zostać wypisane
            // na standardowe wyjście po jednej linii na jeden plik. Każda linia powinna
            // zawierać informację:


            auto lambd = std::function([this](cmds::simpl_cmd ans)->bool {
                // validate cmd - we expect "MY LIST"
                if (ans.get_cmd().compare(cmmn::my_list_) != 0) {
                    return false;
                }

                /* std::string res;
                 std::getline(cin, res);
                 std::vector<std::string> details;
                 boost::split(details, res, boost::is_any_of(","));
                 // If I iterate through the vector there is only one element "John" and not all ?
                 for (std::vector<std::string>::iterator pos = details.begin(); pos != details.end(); ++pos) {
                     cout << *pos << endl;
                 }*/

                //to w jakiejś śmiesznej pętli
                // available_files.insert({ans.get_param(), cmmn::get_ip(sender)});
                return true;
            });

            available_files.clear();
            send_and_recv(cmds::simpl_cmd{cmmn::list_, get_next_seq(), sub}, mcast_addr, seq, lambd);
        }

    };
}

#endif //CLIENT_HPP
