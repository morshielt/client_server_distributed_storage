#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "common.hpp"

namespace cmmn = sik_2::common;


namespace sik_2::sockets {

    class socket_UDP_MCAST {
    private:
        int sock;
        struct sockaddr_in remote_address{};
        struct timeval start;
        int32_t timeout;

    public:
        socket_UDP_MCAST(std::string mcast_addr, int32_t cmd_port, int32_t timeout) : timeout{timeout} {
            //W celu poznania listy aktywnych węzłów serwerowych w grupie klient wysyła na adres rozgłoszeniowy
            //MCAST_ADDR i port CMD_PORT pakiet SIMPL_CMD z poleceniem cmd = “HELLO” oraz pustą zawartością pola data.

            int optval;

            /* otworzenie gniazda */
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0) {
                throw std::system_error(EFAULT, std::generic_category());
            }

            /* uaktywnienie rozgłaszania (ang. broadcast) */
            optval = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &optval, sizeof optval) < 0) {
                throw std::system_error(EFAULT, std::generic_category());
            }

            /* ustawienie TTL dla datagramów rozsyłanych do grupy */
            optval = cmmn::TTL_VALUE;
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &optval, sizeof optval) < 0) {
                throw std::system_error(EFAULT, std::generic_category());
            }

            // // czekanie timeout sekund na odpowiedź
            // struct timeval tv;
            // tv.tv_sec = timeout;
            // tv.tv_usec = 0;
            // setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);

            /* ustawienie adresu i portu odbiorcy */
            remote_address.sin_family = AF_INET;
            remote_address.sin_port = htons(cmd_port);
            if (inet_aton(mcast_addr.c_str(), &remote_address.sin_addr) == 0) {
                throw std::invalid_argument("invalid port/address");
            }
        }

        int get_sock() {
            return sock;
        }

        struct sockaddr_in *get_remote_address() {
            return &remote_address;
        }

        ~socket_UDP_MCAST() {
            close(sock);
        }

        void set_timestamp() {
            gettimeofday(&start, nullptr);
        }

        bool update_timeout() {
            struct timeval curr;
            gettimeofday(&curr, nullptr);

            struct timeval diff;
            diff.tv_sec = curr.tv_sec - start.tv_sec;
            diff.tv_usec = curr.tv_usec - start.tv_usec;
            if (diff.tv_usec < 0) {
                diff.tv_sec--;
                diff.tv_usec += 1000000;
            }

            if (diff.tv_sec > timeout) {
                return false;
            }

            // time left
            diff.tv_sec = timeout - diff.tv_sec - 1;
            diff.tv_usec = 1000000 - diff.tv_usec;

            std::cout << diff.tv_sec << " " << diff.tv_usec << "\n";
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &diff, sizeof diff);

            return true;
        }

    };

    class socket_UDP {
    private:
        int sock;
        struct sockaddr_in local_address{};
        struct ip_mreq ip_mreq{};
        int32_t timeout;


    public:
        socket_UDP(std::string mcast_addr, int32_t cmd_port, int32_t timeout) : timeout{timeout} {

            /* otworzenie gniazda */
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0) {
                throw std::system_error(EFAULT, std::generic_category());
            }

            /* podpięcie się do grupy rozsyłania (ang. multicast) */
            ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            if (inet_aton(mcast_addr.c_str(), &ip_mreq.imr_multiaddr) == 0) {
                throw std::invalid_argument("invalid port/address");
                // syserr("inet_aton");
            }

            if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0) {
                throw std::system_error(EFAULT, std::generic_category());
                // syserr("setsockopt");
            }

            int address_reuse = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &address_reuse, sizeof address_reuse) < 0) {
                throw std::system_error(EFAULT, std::generic_category());
                // syserr("setsockopt");
            }

            /* podpięcie się pod lokalny adres i port */
            local_address.sin_family = AF_INET;
            local_address.sin_addr.s_addr = htonl(INADDR_ANY);
            local_address.sin_port = htons(cmd_port);
            if (bind(sock, (struct sockaddr *) &local_address, sizeof local_address) < 0) {
                throw std::system_error(EFAULT, std::generic_category());
                // syserr("bind");
            }
        }

        int get_sock() {
            return sock;
        }

        ~socket_UDP() {
            /* w taki sposób można odpiąć się od grupy rozsyłania */
            // if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0) {
            //     throw std::system_error(EFAULT, std::generic_category());
            // }

            setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq);
            /* koniec */
            close(sock);
        }

    };
}

#endif //SOCKETS_HPP
