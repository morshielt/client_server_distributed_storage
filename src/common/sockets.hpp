#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include "common.hpp"
#include "validation.hpp"

namespace cmmn = sik_2::common;
namespace valid = sik_2::validation;

namespace sik_2::sockets {

    class socket_ {

    protected:
        int sock;
        int32_t timeout;

        socket_(int32_t timeout) : timeout{timeout} {
            if (!valid::in_range_incl<int32_t>(timeout, 1, cmmn::MAX_TIMEOUT)) {
                throw excpt::invalid_argument{"timeout = " + std::to_string(timeout)};
            }
        }

        void throw_close(int line) {
            std::cout << "throw_close :: " << line << "\n";
            if (sock >= 0) close(sock);
            throw excpt::socket_excpt(std::strerror(errno));
        }

    public:
        virtual int get_sock() {
            return sock;
        }
    };

    class socket_UDP_MCAST : public socket_ {

    private:
        struct sockaddr_in remote_address{};
        struct timeval start;


    public:
        socket_UDP_MCAST(const std::string &mcast_addr, int32_t cmd_port, int32_t timeout)
            : socket_{timeout} {

            /* otworzenie gniazda */
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0)
                throw_close(__LINE__);

            /* uaktywnienie rozgłaszania (ang. broadcast) */
            int optval = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &optval, sizeof optval) < 0)
                throw_close(__LINE__);

            /* ustawienie TTL dla datagramów rozsyłanych do grupy */
            optval = cmmn::TTL_VALUE;
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &optval, sizeof optval) < 0)
                throw_close(__LINE__);

            /* ustawienie adresu i portu odbiorcy */
            remote_address.sin_family = AF_INET;
            remote_address.sin_port = htons(cmd_port);
            if (inet_aton(mcast_addr.c_str(), &remote_address.sin_addr) == 0) {
                if (sock >= 0) close(sock);
                throw std::invalid_argument("invalid port/address");
            }
        }

        struct sockaddr_in *get_remote_address() {
            return &remote_address;
        }

        ~socket_UDP_MCAST() {
            if (sock >= 0) close(sock);
        }

        void set_timestamp() {
            gettimeofday(&start, nullptr);
        }

        // TODO timeout = 0?
        bool update_timeout() {
            struct timeval curr;
            gettimeofday(&curr, nullptr);

            struct timeval diff;
            diff.tv_sec = timeout + start.tv_sec - curr.tv_sec;
            diff.tv_usec = start.tv_usec - curr.tv_usec;

            if (diff.tv_usec < 0) {
                diff.tv_sec--;
                diff.tv_usec += 1000000;
            }

            // TODO czy false jest ok?
            if (diff.tv_sec < 0 || diff.tv_usec < 0) {
                return false;
            }

            // std::cout << diff.tv_sec << " " << diff.tv_usec << "\n";

            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &diff, sizeof diff) < 0)
                throw_close(__LINE__);

            return true;
        }

    };

    class socket_UDP : public socket_ {

    private:
        struct ip_mreq ip_mreq{};

    public:
        socket_UDP(std::string mcast_addr, int32_t cmd_port, int32_t timeout) : socket_{timeout} {

            /* otworzenie gniazda */
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0)
                throw_close(__LINE__);

            /* podpięcie się do grupy rozsyłania (ang. multicast) */
            ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            if (inet_aton(mcast_addr.c_str(), &ip_mreq.imr_multiaddr) == 0) {
                if (sock >= 0) close(sock);
                throw std::invalid_argument("invalid port/address");
            }

            if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0)
                throw_close(__LINE__);

            int address_reuse = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &address_reuse, sizeof address_reuse) < 0)
                throw_close(__LINE__);

            struct sockaddr_in local_address{};
            /* podpięcie się pod lokalny adres i port */
            local_address.sin_family = AF_INET;
            local_address.sin_addr.s_addr = htonl(INADDR_ANY);
            local_address.sin_port = htons(cmd_port);
            // TODO tu się czasem wywala w serwerze jak dłużej działa D:
            if (bind(sock, (struct sockaddr *) &local_address, sizeof local_address) < 0)
                throw_close(__LINE__);
        }

        ~socket_UDP() {
            // TODO to może rzucić, co wtedyyyy? xD nic? ok.
            setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq);
            /* koniec */
            // zamykać jak wychodzę z grupy czy nie D:?
            if (sock >= 0) close(sock);
        }

    };

    namespace {
        const int QUEUE_LENGTH{5};
    }

    class socket_TCP_server : public socket_ {

    private:
        int msg_sock;
        int32_t port;

    public:
        socket_TCP_server(int32_t timeout, std::string addr) : socket_{timeout} {
            struct sockaddr_in server_address;

            sock = socket(PF_INET, SOCK_STREAM, 0); // creating IPv4 TCP socket
            if (sock < 0)
                throw_close(__LINE__);
            // after socket() call; we should close(sock) on any execution path;
            // since all execution paths exit immediately, sock would be closed when program terminates

            server_address.sin_family = AF_INET; // IPv4
            server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
            // std::cout << "\"" << addr.c_str() << "\"\n";
            // server_address.sin_addr.s_addr = inet_addr(addr.c_str());
            server_address.sin_port = htons(0); // listening on port PORT_NUM

            // bind the socket to a concrete address
            if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
                throw_close(__LINE__);

            // // timeout
            // struct timeval diff;
            // diff.tv_sec = timeout;
            // diff.tv_usec = 0;
            // if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &diff, sizeof diff) < 0)
            //     throw_close(__LINE__);

            socklen_t server_address_len = sizeof(server_address);
            if (getsockname(sock, (struct sockaddr *) &server_address, &server_address_len) < 0)
                throw_close(__LINE__);

            port = ntohs(server_address.sin_port);


            // switch to listening (passive open)
            if (listen(sock, QUEUE_LENGTH) < 0)
                throw_close(__LINE__);

            printf("accepting client connections on port %hu\n", ntohs(server_address.sin_port));

            // get client connection from the socket
            // msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);
            //
            // if (msg_sock == -1)
            //     throw_close(__LINE__);
        }

        void get_connection() {
            struct sockaddr_in client_address;
            socklen_t client_address_len;

            client_address_len = sizeof(client_address);



            fd_set set;
            FD_ZERO(&set);
            FD_SET(sock, &set);

            struct timeval tv{};
            tv.tv_sec = timeout;
            tv.tv_usec = 0;

            int ret = select(sock + 1, &set, nullptr, nullptr, &tv);
            if (ret <= 0) {
                throw_close(__LINE__);
            } else {
                msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);

                if (msg_sock < 0) {
                    throw_close(__LINE__);
                }
            }

            // msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);

            // timeout
            tv.tv_sec = timeout;
            tv.tv_usec = 0;
            if (setsockopt(msg_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv) < 0)
                throw_close(__LINE__);

            if (msg_sock == -1)
                throw_close(__LINE__);
        }

        uint32_t get_port() {
            return port;
        }

        uint32_t get_port_be() {
            return htons(port);
        }

        int get_sock() override {
            return msg_sock;
        }

        ~socket_TCP_server() {
            close(msg_sock);
            close(sock);
        }
    };

    class socket_TCP_client : public socket_ {

    private:
        // int32_t port;

    public:
        socket_TCP_client(int32_t timeout, std::string addr, int32_t port) : socket_{timeout} {
            struct addrinfo addr_hints;
            struct addrinfo *addr_result;

            // 'converting' host/port in string to struct addrinfo
            memset(&addr_hints, 0, sizeof(struct addrinfo));
            addr_hints.ai_family = AF_INET; // IPv4
            addr_hints.ai_socktype = SOCK_STREAM;
            addr_hints.ai_protocol = IPPROTO_TCP;

            std::cout << "std::to_string(port).c_str() " << std::to_string(port).c_str() << "\n";
            std::cout << "addr.c_str() " << addr.c_str() << "\n";
            if (getaddrinfo(addr.c_str(), std::to_string(port).c_str(), &addr_hints, &addr_result) != 0) {
                std::cout << __LINE__ << " " << __FILE__ << "\n";
                throw excpt::socket_excpt(std::strerror(errno));
            }

            // initialize socket according to getaddrinfo results
            sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
            if (sock < 0) {
                std::cout << __LINE__ << " " << __FILE__ << "\n";
                throw_close(__LINE__);
            }

            // connect socket to the server
            if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0) {
                std::cout << __LINE__ << " " << __FILE__ << "\n";
                throw_close(__LINE__);
            }

            freeaddrinfo(addr_result);
        }

        ~socket_TCP_client() {
            close(sock);
        }
    };
}

#endif //SOCKETS_HPP
