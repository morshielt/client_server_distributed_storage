#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <iostream>
#include <regex>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>
#include "../common/common.hpp"
#include "../common/commands.hpp"
#include "../common/exceptions.hpp"
#include "../common/validation.hpp"
#include "../common/sockets.hpp"
#include "Request_parser.hpp"

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
        uint64_t seq{};

        std::set<std::pair<uint64_t, std::string>, std::greater<>> servers;
        struct sockaddr sender{};

        reqp::Request_parser req_parser;

        uint64_t get_next_seq() {
            return seq++;
        }

    public:
        Client() = delete;
        Client(const Client &other) = delete;
        Client(Client &&other) = delete;

        Client(const std::string &mcast_addr, const int32_t cmd_port, const std::string &out_fldr, int32_t timeout)
            : mcast_addr{mcast_addr}, cmd_port{cmd_port}, out_fldr{out_fldr}, timeout{timeout} {

            if (!valid::valid_ip(mcast_addr))
                throw excpt::Invalid_argument{"mcast_addr = " + mcast_addr};

            if (!valid::in_range_incl<int32_t>(cmd_port, 0, cmmn::MAX_PORT))
                throw excpt::Invalid_argument{"cmd_port = " + std::to_string(cmd_port)};

            if (!valid::valid_directory(out_fldr))
                throw excpt::Invalid_argument{"out_fldr = " + out_fldr};
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
            if (ret == -1 && !(errno == EWOULDBLOCK || errno == EAGAIN))
                throw excpt::Socket_excpt(std::strerror(errno));

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
                        std::cout << "TEGES???\n";
                        break;
                    }
                    std::cout << "TEGES 22222222222 ???\n";

                    throw excpt::Socket_excpt(std::strerror(errno));
                }

                // check cmd & cmd_seq
                Recv_type y{buffer, static_cast<uint64_t>(ret)};
                if (cmmn::DEBUG) y.print_bytes();

                // validate cmd_seq & lambda result
                if (y.get_cmd_seq() != cmd_seq || !func(y)) {
                    std::cout << "[PCKG ERROR] Skipping invalid package from "
                              << cmmn::get_ip(sender) << ":" << cmd_port << "\n";
                }
            }
            // assert(teges == false); no tak to nigdy nie będzie przez tego break'a

        }

        void do_discover(bool upload) {

            auto lambd = std::function([this, upload](cmds::Cmplx_cmd ans)->bool {
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
            send_and_recv(cmds::Simpl_cmd{cmmn::hello_, get_next_seq(), ""}, mcast_addr, seq, lambd);
        }

        // TODO nowy thread na do_upload~
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
                    // validate answers - we expect "NO_WAY" or "CAN_ADD"
                    if (ans.get_cmd().compare(cmmn::no_way_) == 0) {
                        return (ans.get_data().compare(filename) == 0);

                    } else if (ans.get_cmd().compare(cmmn::can_add_)) {
                        accept = true;
                        //TODO tcp
                        return true;
                    } else { // unknown cmd
                        return false;
                    }
                });

                // while nobody has accepted our "ADD" request, we keep trying
                if (!accept) {
                    send_and_recv(cmds::Cmplx_cmd{cmmn::add_, get_next_seq(), fs::file_size(out_fldr + filename),
                                                  filename},
                                  ser.second, seq, lambd);
                } else {
                    break;
                }
            }
        }
    };
}

#endif //CLIENT_HPP
