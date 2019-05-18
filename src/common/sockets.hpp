#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.hpp"

namespace cmmn = sik_2::common;


namespace sik_2::sockets {
    class socket_UPD_MCAST {
    private:
        int sock;
        struct sockaddr_in remote_address{};

    public:
        socket_UPD_MCAST(std::string mcast_addr, int32_t cmd_port) {
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

        ~socket_UPD_MCAST() {
            close(sock);
        }
    };
}

#endif //SOCKETS_HPP
