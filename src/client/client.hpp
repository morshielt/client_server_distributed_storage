#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <iostream>
#include <regex>
#include <thread>
#include <atomic>
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


// TODO errno == EINTR w TCP - wtedy jeszcze raz próbować czytać

// popaczeć timeouty, na ready, write'y podopisywać bo się zawiesi
// SIGPIPE ogarnąć~
// TODO nowe sockety dla fetcha
namespace sik_2::client {

    bool valid_directory(std::string path) {
        try {
            if (cmmn::DEBUG) {
                const fs::file_status file = fs::status(path);
                std::cout << "file.type() " << file.type() << "\n";
                std::cout << "file.permissions() " << file.permissions() << "\n";
            }

            fs::directory_iterator i(path); // działa jak próba otworzenia, wywala perm denied jak trzeba
            return fs::is_directory(path);
        } catch (...) {
            throw;
        }
    }

    class client {

    private:
        std::string mcast_addr;
        int32_t cmd_port;
        std::string out_fldr;
        int32_t timeout;
        std::atomic<uint64_t> seq{};

        std::set<std::pair<uint64_t, std::string>, std::greater<>> servers{};
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

            if (!valid_directory(out_fldr))
                throw excpt::invalid_argument{"out_fldr = " + out_fldr};
        }

        void run() {

            std::string param{};

            while (true) {
                std::cout << "param :: \"" << param << "\"\n";

                switch (req_parser.next_request(param)) {
                    case cmmn::Request::discover: {
                        if (cmmn::DEBUG) std::cout << "DISCOVER" << "\n";
                        // std::thread t{[this] { do_discover(false); }};
                        // t.detach();
                        do_discover(false);
                        break;
                    }
                    case cmmn::Request::search: {
                        if (cmmn::DEBUG) std::cout << "SEARCH" << "\n";
                        // std::thread t{[this, &param] { do_search(param); }};
                        // t.detach();
                        do_search(param);
                        break;
                    }
                    case cmmn::Request::fetch: {
                        if (cmmn::DEBUG) std::cout << "FETCH" << "\n";
                        if (is_fetchable(param)) {
                            // std::thread t{[this, &param] { do_fetch(param); }};
                            // t.detach();
                            do_fetch(param);
                        } else std::cout << "WRONK FETCH\n";
                        break;
                    }
                    case cmmn::Request::upload: {
                        if (cmmn::DEBUG) std::cout << "UPLOAD" << "\n";
                        // std::thread t{[this, &param] { do_upload(param); }};
                        // t.detach();
                        do_upload(param);
                        break;
                    }
                    case cmmn::Request::remove: {
                        if (cmmn::DEBUG) std::cout << "REMOVE" << "\n";
                        // std::thread t{[this, &param] { do_remove(param); }};
                        // t.detach();
                        do_remove(param);
                        break;
                    }
                    case cmmn::Request::exit: { // TODO zamyka połączenia? jak bd wątki to bd ok czy nie? xD
                        if (cmmn::DEBUG) std::cout << "EXIT" << "\n";
                        return;
                        break;
                    }
                    default: {
                        std::cout << "UNKNOWN\n";
                    }
                }
            }
        }

    private:
        template<typename Send_type, typename Recv_type>
        void send_and_recv(Send_type s_cmd, const std::string &addr, uint64_t cmd_seq,
                           std::function<bool(Recv_type ans)> &func, struct sockaddr &sender) {

            char buffer[cmmn::MAX_UDP_PACKET_SIZE];
            ssize_t ret{0};

            // TODO nowy socket tylko dla fetcha,
            // nie dla każdego, lol XD
            sckt::socket_UDP_MCAST udpm_sock{addr, cmd_port, timeout};

            // send request
            ret = sendto(udpm_sock.get_sock(), s_cmd.get_raw_msg(), s_cmd.get_msg_size(), 0, (struct sockaddr *)
                udpm_sock.get_remote_address(), sizeof(*udpm_sock.get_remote_address()));

            // TODO czy my w ogóle chcemy ifować EWOULDBLOCK i EAGAIN?
            // czy my chcemy przy wysyłaniu? czy tylko receivie?
            if (ret == -1 && !(errno == EWOULDBLOCK || errno == EAGAIN)) {
                std::cout << __LINE__ << " " << __FILE__ << "\n";
                throw excpt::socket_excpt(std::strerror(errno));
            }

            // wait 'timeout' seconds for answers
            udpm_sock.set_timestamp();

            while (udpm_sock.update_timeout()) {

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
            struct sockaddr sender{};

            auto lambd = std::function([this, upload, &sender](cmds::cmplx_cmd ans)->bool {
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
            uint64_t curr_seq = get_next_seq();
            send_and_recv(cmds::simpl_cmd{cmmn::hello_, curr_seq, ""}, mcast_addr, curr_seq, lambd, sender);
            std::cout << "jest po do_discover.\n";
        }

        // TODO nowy thread na do_shit~
        void do_upload(const std::string &filename) {
            struct sockaddr sender{};

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

                    auto lambd = std::function([this, &accept, &sender, filename, file_size](cmds::simpl_cmd ans)
                                                   ->bool {
                        // validate answers - we expect "NO_WAY" or "CAN_ADD"
                        if (ans.get_cmd().compare(cmmn::no_way_) == 0) {

                            return (ans.get_data().compare(filename) == 0);

                        } else if (ans.get_cmd().compare(cmmn::can_add_) == 0) {
                            accept = true;

                            std::thread t{[this, sender, filename, file_size, ans] {
                                cmds::cmplx_cmd cst{ans.get_raw_msg(), ans.get_msg_size()};
                                sckt::socket_TCP_client
                                    tcp_sock{timeout, cmmn::get_ip(sender), (int32_t) cst.get_param()};

                                try {
                                    cmmn::send_file(cmmn::get_path(out_fldr, filename), file_size, tcp_sock.get_sock());
                                    std::cout << "File " << filename << " uploaded ("
                                              << cmmn::get_ip(sender) << ":" << (int32_t) cst.get_param()
                                              << ")\n";
                                } catch (excpt::file_excpt &e) {
                                    std::cout << "File " << filename << " uploading failed ("
                                              << cmmn::get_ip(sender) << ":" << (int32_t) cst.get_param()
                                              << ") " << e.what() << "\n";
                                }
                            }};
                            t.detach();


                            return true;
                        } else { // unknown cmd
                            return false;
                        }
                    });

                    // while nobody has accepted our "ADD" request, we keep trying
                    if (!accept) {
                        std::cout << "filesize :: " << file_size << "\n";
                        uint64_t curr_seq = get_next_seq();
                        send_and_recv(cmds::cmplx_cmd{cmmn::add_, curr_seq, file_size, filename},
                                      ser.second, curr_seq, lambd, sender);
                    } else {
                        break;
                    }
                }
            }
            std::cout << "jest po do_upload.\n";
        }

        void do_search(const std::string &sub) {
            struct sockaddr sender{};
            auto lambd = std::function([this, &sender](cmds::simpl_cmd ans)->bool {
                if (ans.get_cmd().compare(cmmn::my_list_) != 0) {
                    return false;
                }

                fill_files_list(ans.get_data(), sender);

                return true;
            });

            available_files.clear();
            uint64_t curr_seq = get_next_seq();
            send_and_recv(cmds::simpl_cmd{cmmn::list_, curr_seq, sub}, mcast_addr, curr_seq, lambd, sender);
            std::cout << "jest po do_search.\n";
        }

        void fill_files_list(std::string list, struct sockaddr sender) {
            std::string sender_ip = cmmn::get_ip(sender);
            do {
                std::string tmp = std::string{list, 0, list.find(cmmn::SEP)};
                std::cout << tmp << " (" << sender_ip << ")\n";
                // std::cout << "list " << list << "\n";
                available_files.insert({tmp, cmmn::get_ip(sender)});
                list = std::string{list, list.find(cmmn::SEP) + 1, list.length()};
            } while (list.length() > 0 && list.find(cmmn::SEP) != std::string::npos);

            std::cout << list << "\n";
            available_files.insert({list, cmmn::get_ip(sender)});
        }

        void do_fetch(const std::string &filename) {
            struct sockaddr sender{};

            auto lambd = std::function([this, &sender, filename](cmds::cmplx_cmd ans)->bool {
                if (ans.get_cmd().compare(cmmn::connect_me_) != 0) {
                    return false;
                }

                assert(available_files.find(filename)->second == cmmn::get_ip(sender));

                std::thread t{[this, sender, ans, filename] {
                    sckt::socket_TCP_client tcp_sock{timeout, cmmn::get_ip(sender), (int32_t) ans.get_param()};

                    try {
                        cmmn::receive_file(cmmn::get_path(out_fldr, filename), tcp_sock.get_sock());
                        std::cout << "File " << filename << " downloaded ("
                                  << cmmn::get_ip(sender) << ":" << (int32_t) ans.get_param()
                                  << ")\n";
                    } catch (excpt::file_excpt &e) {
                        std::cout << "File " << filename << " downloading failed ("
                                  << cmmn::get_ip(sender) << ":" << (int32_t) ans.get_param()
                                  << ") " << e.what() << "\n";
                    }
                }};
                t.detach();

                return true;
            });


            std::cout << "available_files.find(filename)->second " << available_files.find(filename)->second << "\n";
            uint64_t curr_seq = get_next_seq();
            send_and_recv(cmds::simpl_cmd{cmmn::get_, curr_seq, filename},
                          available_files.find(filename)->second, curr_seq, lambd, sender);
            std::cout << "jest po do_fetch.\n";
        }

        bool is_fetchable(const std::string &name) {
            return available_files.find(name) != available_files.end();
        }

        void do_remove(const std::string &filename) {

            sckt::socket_UDP_MCAST udpm_sock{mcast_addr, cmd_port, timeout};
            uint64_t curr_seq = get_next_seq();
            cmds::simpl_cmd s_cmd{cmmn::del_, curr_seq, filename};
            // send request
            int ret = 0;
            ret = sendto(udpm_sock.get_sock(), s_cmd.get_raw_msg(), s_cmd.get_msg_size(), 0, (struct sockaddr *)
                udpm_sock.get_remote_address(), sizeof(*udpm_sock.get_remote_address()));

            // TODO czy my w ogóle chcemy ifować EWOULDBLOCK i EAGAIN?
            // chcemy, ale czy przy sendzie chcemy?
            if (ret == -1 && !(errno == EWOULDBLOCK || errno == EAGAIN)) {
                std::cout << __LINE__ << " " << __FILE__ << "\n";
                throw excpt::socket_excpt(std::strerror(errno));
            }
            std::cout << "jest po do_remove.\n";
        }
    };
}

#endif //CLIENT_HPP
