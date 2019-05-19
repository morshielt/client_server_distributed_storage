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
#include <set>

namespace fs =  boost::filesystem;
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

        std::set<std::pair<uint64_t, std::string>, std::greater<>> servers;
        struct sockaddr sender{};

        reqp::Request_parser req_parser;

        std::string get_sender_ip() {
            return inet_ntoa(((struct sockaddr_in *) &sender)->sin_addr);
        }

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
                        do_discover(false);
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
                        do_upload(param);
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

        // void send_cmd(cmds::Simpl_cmd &s_cmd, sckt::socket_UDP_MCAST &sock) {
        //     // sckt::socket_UDP_MCAST sock{mcast_addr, cmd_port, timeout};
        //
        //     int bytes_sent =
        //         sendto(sock.get_sock(), s_cmd.raw_msg(), s_cmd.msg_size(), 0,
        //                (struct sockaddr *) sock.get_remote_address(), sizeof(*sock.get_remote_address()));
        //     std::cout << "bytes_sent " << bytes_sent << "\n";
        // }

        template<typename Send_type, typename Recv_type>
        void send_and_recv(Send_type s_cmd, std::string addr, uint64_t cmd_seq,
                           std::function<bool(Recv_type ans)> &func) {

            sckt::socket_UDP_MCAST sock{addr, cmd_port, timeout};

            int bytes_sent =
                sendto(sock.get_sock(), s_cmd.raw_msg(), s_cmd.msg_size(), 0,
                       (struct sockaddr *) sock.get_remote_address(), sizeof(*sock.get_remote_address()));
            std::cout << "bytes_sent " << bytes_sent << "\n";

            // struct sockaddr sender{};
            uint64_t rcv_len{};

            char buffer[cmmn::MAX_UDP_PACKET_SIZE];

            // if (upload) servers.clear();
            sock.set_timestamp();

            while (sock.update_timeout()) {
                socklen_t sendsize = sizeof(sender);
                memset(&sender, 0, sizeof(sender));
                rcv_len =
                    recvfrom(sock.get_sock(), buffer, sizeof(buffer), 0, &sender, &sendsize);

                if (rcv_len == -1) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        break;
                    }
                    throw std::system_error(EFAULT, std::generic_category());
                } else {
                    std::cout << "rcv_len " << rcv_len << "\n";
                    cmmn::print_bytes(rcv_len, buffer);
                }

                // check cmd & cmd_seq
                Recv_type y{buffer, rcv_len};

                std::cout << "OUOUOUOUO " << y.get_cmd_seq() << " " << cmd_seq << "\n";
                if (y.get_cmd_seq() != cmd_seq || !func(y)) {
                    std::cout << "[PCKG ERROR] Skipping invalid package from "
                              << get_sender_ip() << ":" << cmd_port << "\n";
                }
            }
        }

        void do_discover(bool upload) {
            auto lambd = std::function([this, upload](cmds::Cmplx_cmd ans)->bool {
                std::cout << "OUOUOUOUO \"" << ans.get_cmd() << "\" \"" << cmmn::good_day_ << "\"\n";

                // int compare(size_t pos, size_t len, const string &str) const;
                std::cout << "TRU? " << ans.get_cmd().compare(/*0, cmmn::CMD_SIZE, */cmmn::good_day_) << "\n";
                if (ans.get_cmd().compare(/*0, cmmn::CMD_SIZE, */cmmn::good_day_) != 0) {
                    return false;
                }

                if (!upload) {
                    std::cout << "Found " << get_sender_ip() << " (" << ans.get_data() << ") with free space "
                              << ans.get_param() << "\n";
                } else {
                    servers.insert({ans.get_param(), get_sender_ip()});
                }
                return true;
            });

            if (upload) servers.clear();
            send_and_recv(cmds::Simpl_cmd{cmmn::hello_, 666, "BANAN"}, mcast_addr, 666, lambd);
        }

        void do_upload(const std::string &filename) {

            // TODO sprawdzenie czy dodać '/'
            if (!fs::exists(out_fldr + filename)) {
                std::cerr << "File " << filename << " does not exist\n";
                return;
            }

            do_discover(true);

            if (fs::file_size(out_fldr + filename) > servers.begin()->first) {
                std::cerr << "File " << filename << " too big\n";
                return;
            }

            bool accept = false;
            for (auto &ser : servers) {

                auto lambd = std::function([this, &accept, filename](cmds::Cmplx_cmd ans)->bool {

                    std::cout << "CAN ADD? " << ans.get_cmd().compare(cmmn::can_add_) << "\n";
                    std::cout << "NO WAY? " << ans.get_cmd().compare(cmmn::no_way_) << "\n";

                    if (ans.get_cmd().compare(cmmn::no_way_) == 0) {
                        return (ans.get_data().compare(filename) == 0);

                    } else if (ans.get_cmd().compare(cmmn::can_add_)) {
                        accept = true;
                        //TODO tcp
                        return true;

                    } else {
                        return false;
                    }
                });

                if (!accept) {
                    send_and_recv(cmds::Cmplx_cmd{cmmn::add_, 666, fs::file_size(out_fldr + filename), filename}, ser
                        .second, 666, lambd);
                } else {
                    break;
                }
            }
        }

    };
}

#endif //CLIENT_HPP
