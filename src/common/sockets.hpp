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
                throw excpt::Invalid_argument{"timeout = " + std::to_string(timeout)};
            }
        }

        void throw_close(int line) {
            std::cout << "throw_close :: " << line << "\n";
            if (sock >= 0) close(sock);
            throw excpt::Socket_excpt(std::strerror(errno));
        }

    public:
        int get_sock() {
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
            // TODO kiedy to zwraca false? XD
            struct timeval curr;
            gettimeofday(&curr, nullptr);

            struct timeval diff;
            diff.tv_sec = timeout + start.tv_sec - curr.tv_sec;
            diff.tv_usec = start.tv_usec - curr.tv_usec;

            if (diff.tv_usec < 0) {
                diff.tv_sec--;
                diff.tv_usec += 1000000;
            }

            std::cout << diff.tv_sec << " " << diff.tv_usec << "\n";

            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &diff, sizeof diff) < 0)
                throw_close(__LINE__);

            return true;
        }

    };

    class socket_UDP : public socket_ {

    private:
        struct sockaddr_in local_address{};
        struct ip_mreq ip_mreq{};
        struct sockaddr sender{};

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


            /* podpięcie się pod lokalny adres i port */
            local_address.sin_family = AF_INET;
            local_address.sin_addr.s_addr = htonl(INADDR_ANY);
            local_address.sin_port = htons(cmd_port);
            if (bind(sock, (struct sockaddr *) &local_address, sizeof local_address) < 0)
                throw_close(__LINE__);
        }

        void set_sender(struct sockaddr addr) {
            sender = addr;
        }

        struct sockaddr get_sender() {
            return sender;
        }

        ~socket_UDP() {
            // todo ?
            /* w taki sposób można odpiąć się od grupy rozsyłania */
            // if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0) {
            //     throw std::system_error(EFAULT, std::generic_category());
            // }

            setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq);
            /* koniec */
            // zamykać jak wychodzę z grupy czy nie D:?
            if (sock >= 0) close(sock);
        }

    };

    namespace {
        const int QUEUE_LENGTH{5};
    }

    class socket_TCP_in : public socket_ {

    private:
        int msg_sock;
        int32_t port;

    public:
        socket_TCP_in(int32_t timeout, std::string addr) : socket_{timeout} {
            struct sockaddr_in server_address;

            sock = socket(PF_INET, SOCK_STREAM, 0); // creating IPv4 TCP socket
            if (sock < 0)
                throw_close(__LINE__);
            // after socket() call; we should close(sock) on any execution path;
            // since all execution paths exit immediately, sock would be closed when program terminates

            server_address.sin_family = AF_INET; // IPv4
            // server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
            std::cout << "\"" << addr.c_str() << "\"\n";
            server_address.sin_addr.s_addr = inet_addr(addr.c_str());
            server_address.sin_port = htons(0); // listening on port PORT_NUM

            // bind the socket to a concrete address
            if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
                throw_close(__LINE__);
            // switch to listening (passive open)
            if (listen(sock, QUEUE_LENGTH) < 0)
                throw_close(__LINE__);

            // timeout
            struct timeval diff;
            diff.tv_sec = timeout;
            diff.tv_usec = 0;
            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &diff, sizeof diff) < 0)
                throw_close(__LINE__);

            socklen_t server_address_len = sizeof(server_address);
            if (getsockname(sock, (struct sockaddr *) &server_address, &server_address_len) < 0)
                throw_close(__LINE__);

            port = ntohs(server_address.sin_port);

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
            msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);

            if (msg_sock == -1)
                throw_close(__LINE__);
        }

        uint32_t get_port() {
            return port;
        }

        ~socket_TCP_in() {
            close(msg_sock);
            close(sock);
        }

        // void send_bytes(char *buffer, uint32_t expected) {
        //     errno = 0;
        //     ssize_t bytes = send(msg_sock, buffer, expected, MSG_NOSIGNAL);
        //
        //     if (errno != 0 || bytes < expected) {
        //         throw_close(__LINE__);
        //     }
        // }
        //
        // void recieve_bytes(char *buffer, uint32_t expected) {
        //     // TODO ????????????? co jak 0??????
        //     if (expected == 0) {
        //         printf("recv 0 bytes, error.\n");
        //         throw_close(__LINE__);
        //     }
        //
        //     ssize_t bytes = recv(msg_sock, buffer, expected, MSG_WAITALL);
        //
        //     if (bytes < expected) {
        //         throw_close(__LINE__);
        //     }
        // }
    };

    class socket_TCP_out : public socket_ {

    private:
        int msg_sock;
        // int32_t port;

    public:
        socket_TCP_out(int32_t timeout, std::string addr, int32_t port) : socket_{timeout} {
            struct addrinfo addr_hints;
            struct addrinfo *addr_result;

            // 'converting' host/port in string to struct addrinfo
            memset(&addr_hints, 0, sizeof(struct addrinfo));
            addr_hints.ai_family = AF_INET; // IPv4
            addr_hints.ai_socktype = SOCK_STREAM;
            addr_hints.ai_protocol = IPPROTO_TCP;
            if (getaddrinfo(addr.c_str(), std::to_string(port).c_str(), &addr_hints, &addr_result) != 0) {
                throw excpt::Socket_excpt(std::strerror(errno));
            }

            // initialize socket according to getaddrinfo results
            sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
            if (sock < 0)
                throw_close(__LINE__);

            // connect socket to the server
            if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
                throw_close(__LINE__);

            freeaddrinfo(addr_result);

        }

        ~socket_TCP_out() {
            close(sock);
        }

        // void send_bytes(char *buffer, uint32_t expected) {
        //     errno = 0;
        //     ssize_t bytes = send(sock, buffer, expected, MSG_NOSIGNAL);
        //
        //     if (errno != 0 || bytes < expected) {
        //         throw_close(__LINE__);
        //     }
        // }
        //
        // void recieve_bytes(char *buffer, uint32_t expected) {
        //     // TODO ????????????? co jak 0??????
        //     if (expected == 0) {
        //         printf("recv 0 bytes, error.\n");
        //         throw_close(__LINE__);
        //     }
        //
        //     ssize_t bytes = recv(sock, buffer, expected, MSG_WAITALL);
        //
        //     if (bytes < expected) {
        //         throw_close(__LINE__);
        //     }
        // }
    };
}

#endif //SOCKETS_HPP
