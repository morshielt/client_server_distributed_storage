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

namespace sik_2::client {

    class client {

    public:
        client(const std::string &mcast_addr, const int32_t cmd_port, const std::string &out_fldr, int32_t timeout)
            : mcast_addr{mcast_addr}, cmd_port{cmd_port}, out_fldr{out_fldr}, timeout{timeout} {

            valid::validate_ip(mcast_addr);

            if (!valid::in_range_inclusive<int32_t>(cmd_port, 0, cmmn::MAX_PORT))
                throw excpt::invalid_argument{"cmd_port = " + std::to_string(cmd_port)};

            validate_directory(out_fldr);
        }

        // Main function - decide which action to perform based on command line input
        void run() {
            std::string param{};

            while (true) {
                switch (req_parser.next_request(param)) {
                    case cmmn::Request::discover: {
                        do_discover(false);
                        break;

                    }
                    case cmmn::Request::search: {
                        do_search(param);
                        break;

                    }
                    case cmmn::Request::fetch: {
                        if (is_fetchable(param))
                            do_fetch(param);
                        break;

                    }
                    case cmmn::Request::upload: {
                        do_upload(param);
                        break;

                    }
                    case cmmn::Request::remove: {
                        do_remove(param);
                        break;

                    }
                    case cmmn::Request::exit: {
                        return;
                    }
                    case cmmn::Request::unknown: {
                        break;
                    }
                }
            }
        }

    private:
        // Sending request and handling answer template function
        template<typename Send_type, typename Recv_type>
        void send_and_recv(Send_type s_cmd, const std::string &addr, uint64_t cmd_seq,
                           std::function<bool(Recv_type ans)> &answer_handler, struct sockaddr &sender) {

            char buffer[cmmn::MAX_UDP_PACKET_SIZE];
            ssize_t ret{0};

            sckt::socket_UDP_client udpm_sock{addr, cmd_port, timeout};

            // send request
            ret = sendto(udpm_sock.get_sock(), s_cmd.get_raw_msg(), s_cmd.get_msg_size(), 0, (struct sockaddr *)
                udpm_sock.get_remote_address(), sizeof(*udpm_sock.get_remote_address()));

            if (ret == -1 && !(errno == EWOULDBLOCK || errno == EAGAIN)) {
                throw excpt::socket_excpt(std::strerror(errno));
            }

            // wait 'timeout' seconds for answers
            udpm_sock.set_timestamp();
            while (udpm_sock.update_timeout()) {

                socklen_t sendsize = sizeof(sender);
                memset(&sender, 0, sizeof(sender));
                ret = recvfrom(udpm_sock.get_sock(), buffer, sizeof(buffer), 0, &sender, &sendsize);

                if (ret == -1) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        break;
                    }
                    throw excpt::socket_excpt(std::strerror(errno));
                }

                // check cmd & cmd_seq
                Recv_type y{buffer, static_cast<uint64_t>(ret)};

                // validate cmd_seq & try to handle answer in 
                // 'answer_handler' (function given in parameter)
                if (y.get_cmd_seq() != cmd_seq || !answer_handler(y)) {
                    std::string tmp = "[PCKG ERROR] Skipping invalid package from "
                        + cmmn::get_ip(sender) + ":" + std::to_string(cmd_port) + "\n";
                    std::cerr << tmp;
                }
            }
        }

        uint64_t get_next_seq() {
            return seq++;
        }

        // Send discover request - if 'upload', then it's a call for uploading purposes,
        // so we only fill servers list, if 'upload' is false, it is a call from command line
        void do_discover(bool upload) {
            struct sockaddr sender{};

            auto answer_handler = std::function([this, upload, &sender](cmds::cmplx_cmd ans)->bool {

                if (ans.get_cmd().compare(cmmn::good_day_) != 0) {
                    return false;
                }

                if (!upload) {
                    std::string tmp = "Found " + cmmn::get_ip(sender) + " (" + ans.get_data() + ") with free space "
                        + std::to_string(ans.get_param()) + "\n";
                    std::cout << tmp;
                } else {
                    servers.insert({ans.get_param(), cmmn::get_ip(sender)});
                }
                return true;
            });

            if (upload) servers.clear();
            uint64_t curr_seq = get_next_seq();
            send_and_recv(cmds::simpl_cmd{cmmn::hello_, curr_seq, ""}, mcast_addr, curr_seq, answer_handler, sender);
        }

        // Discover available servers & try to upload file
        void do_upload(const std::string &filename) {
            struct sockaddr sender{};

            if (!fs::exists(cmmn::get_path(out_fldr, filename))) {

                std::string tmp = "File " + filename + " does not exist\n";
                std::cout << tmp;
                return;
            }

            do_discover(true);

            uint64_t file_size = fs::file_size(cmmn::get_path(out_fldr, filename));
            bool accept = false;

            if (!servers.empty()) {
                for (auto &ser : servers) {

                    auto answer_handler =
                        std::function([this, &accept, &sender, filename, file_size](cmds::simpl_cmd ans)
                                          ->bool {

                            if (ans.get_cmd().compare(cmmn::no_way_) == 0) {
                                return (ans.get_data().compare(filename) == 0);
                            } else if (ans.get_cmd().compare(cmmn::can_add_) == 0) {
                                accept = true;

                                // Establish TCP connection
                                std::thread t{[this, sender, filename, file_size, ans] {
                                    cmds::cmplx_cmd cst{ans.get_raw_msg(), ans.get_msg_size()};
                                    sckt::socket_TCP_client
                                        tcp_sock{timeout, cmmn::get_ip(sender), (int32_t) cst.get_param()};

                                    try {
                                        cmmn::send_file(out_fldr, filename, file_size, tcp_sock.get_sock());
                                        std::string tmp = "File " + filename + " uploaded ("
                                            + cmmn::get_ip(sender) + ":" + std::to_string((int32_t) cst.get_param())
                                            + ")\n";
                                        std::cout << tmp;
                                    } catch (excpt::file_excpt &e) {
                                        std::string tmp = "File " + filename + " uploading failed ("
                                            + cmmn::get_ip(sender) + ":" + std::to_string((int32_t) cst.get_param())
                                            + ") " + e.what() + "\n";
                                        std::cout << tmp;
                                    }
                                }};
                                t.detach();

                                return true;
                            } else {
                                return false;
                            }
                        });

                    // while nobody has accepted our "ADD" request, we keep trying
                    if (!accept) {
                        uint64_t curr_seq = get_next_seq();
                        send_and_recv(cmds::cmplx_cmd{cmmn::add_, curr_seq, file_size, filename},
                                      ser.second, curr_seq, answer_handler, sender);
                    } else {
                        break;
                    }
                }
            }

            // Nobody accepted our request
            if (!accept) {
                std::string tmp = "File " + filename + " too big\n";
                std::cout << tmp;
                return;
            }
        }

        // Search filenames containing substring on all servers
        void do_search(const std::string &sub) {
            struct sockaddr sender{};
            auto answer_handler = std::function([this, &sender](cmds::simpl_cmd ans)->bool {
                if (ans.get_cmd().compare(cmmn::my_list_) != 0) {
                    return false;
                }

                if (ans.get_data().compare("\n") != 0)
                    fill_files_list(ans.get_data(), sender);

                return true;
            });

            available_files.clear();
            uint64_t curr_seq = get_next_seq();
            send_and_recv(cmds::simpl_cmd{cmmn::list_, curr_seq, sub}, mcast_addr, curr_seq, answer_handler, sender);
        }

        // Save list of searched files for next fetch requests
        void fill_files_list(std::string list, struct sockaddr sender) {
            std::string sender_ip = cmmn::get_ip(sender);
            while (list.length() > 0 && list.find(cmmn::SEP) != std::string::npos) {

                std::string tmp = std::string{list, 0, list.find(cmmn::SEP)};
                std::string tmp_cout = tmp + " (" + sender_ip + ")\n";
                std::cout << tmp_cout;
                available_files.insert({tmp, cmmn::get_ip(sender)});
                list = std::string{list, list.find(cmmn::SEP) + 1, list.length()};
            }

            available_files.insert({list, cmmn::get_ip(sender)});
        }

        // Fetch file (if it's name was present in last search result)
        void do_fetch(const std::string &filename) {
            struct sockaddr sender{};

            auto answer_handler = std::function([this, &sender, filename](cmds::cmplx_cmd ans)->bool {
                if (ans.get_cmd().compare(cmmn::connect_me_) != 0) {
                    return false;
                }

                // Establish TCP conection
                std::thread t{[this, sender, ans, filename] {
                    sckt::socket_TCP_client tcp_sock{timeout, cmmn::get_ip(sender), (int32_t) ans.get_param()};

                    try {
                        cmmn::receive_file(out_fldr, filename, tcp_sock.get_sock());
                        std::string tmp = "File " + filename + " downloaded ("
                            + cmmn::get_ip(sender) + ":" + std::to_string(ans.get_param()) + ")\n";
                        std::cout << tmp;
                    } catch (excpt::file_excpt &e) {
                        std::string tmp = "File " + filename + " downloading failed ("
                            + cmmn::get_ip(sender) + ":" + std::to_string(ans.get_param()) + ") " + e.what() + "\n";
                        std::cout << tmp;

                    }
                }};
                t.detach();
                return true;
            });

            uint64_t curr_seq = get_next_seq();
            send_and_recv(cmds::simpl_cmd{cmmn::get_, curr_seq, filename},
                          available_files.find(filename)->second, curr_seq, answer_handler, sender);
        }

        bool is_fetchable(const std::string &name) {
            return available_files.find(name) != available_files.end();
        }

        // Send remove request by multicast
        void do_remove(const std::string &filename) {

            sckt::socket_UDP_client udpm_sock{mcast_addr, cmd_port, timeout};
            uint64_t curr_seq = get_next_seq();
            cmds::simpl_cmd s_cmd{cmmn::del_, curr_seq, filename};

            int ret = 0;
            ret = sendto(udpm_sock.get_sock(), s_cmd.get_raw_msg(), s_cmd.get_msg_size(), 0, (struct sockaddr *)
                udpm_sock.get_remote_address(), sizeof(*udpm_sock.get_remote_address()));

            if (ret == -1 && !(errno == EWOULDBLOCK || errno == EAGAIN)) {
                throw excpt::socket_excpt(std::strerror(errno));
            }
        }

        // Check if directory given in program arguments is able to be opened
        // and we have sufficient permissions
        bool validate_directory(std::string path) {
            try {
                fs::directory_iterator i(path);
                return fs::is_directory(path);
            } catch (...) {
                throw excpt::invalid_argument{"out_fldr = " + path};
            }
        }

    private:
        std::string mcast_addr;
        int32_t cmd_port;
        std::string out_fldr;
        int32_t timeout;
        std::atomic<uint64_t> seq{};

        std::set<std::pair<uint64_t, std::string>, std::greater<>> servers{};
        std::map<std::string, std::string> available_files{}; // filename -> ip

        reqp::request_parser req_parser;
    };
}

#endif //CLIENT_HPP
