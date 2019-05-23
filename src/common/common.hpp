#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <regex>
#include <iostream>
#include <cassert>
#include <boost/filesystem.hpp>
#include <netinet/in.h>
#include <sys/socket.h>

#include "exceptions.hpp"

namespace fs = boost::filesystem;
namespace excpt = sik_2::exceptions;

namespace sik_2::common {

    static const int DEBUG{1};
    static const int32_t DEF_TIMEOUT{5};
    static const int32_t MAX_TIMEOUT{300};
    static const int32_t DEF_SPACE{52428800};
    static const int32_t MAX_PORT{65535};
    static const int32_t TTL_VALUE{4};
    // static const int32_t MAX_UDP_PACKET_SIZE{65507};
    static const int32_t MAX_UDP_PACKET_SIZE{65507}; // TODO -S_OFFSET lub  -C_OFFSET, ughr~
    static const int32_t BUFFER_SIZE{512 * 1024}; // TODO -S_OFFSET lub  -C_OFFSET, ughr~
    static const size_t CMD_SIZE = 10;

    static const char SEP = '\n';
    static const int ERROR = -1;

    // commands
    const std::string hello_{"HELLO\0\0\0\0\0", CMD_SIZE};
    const std::string good_day_{"GOOD_DAY\0\0", CMD_SIZE};
    const std::string list_{"LIST\0\0\0\0\0\0", CMD_SIZE};
    const std::string my_list_{"MY_LIST\0\0\0", CMD_SIZE};
    const std::string get_{"GET\0\0\0\0\0\0\0", CMD_SIZE};
    const std::string del_{"DEL\0\0\0\0\0\0\0", CMD_SIZE};
    const std::string connect_me_{"CONNECT_ME", CMD_SIZE};
    const std::string add_{"ADD\0\0\0\0\0\0\0", CMD_SIZE};
    const std::string no_way_{"NO_WAY\0\0\0\0", CMD_SIZE};
    const std::string can_add_{"CAN_ADD\0\0\0", CMD_SIZE};

    enum class Request { search, discover, fetch, upload, remove, exit, unknown };

    std::string get_ip(struct sockaddr addr) {
        return inet_ntoa(((struct sockaddr_in *) &addr)->sin_addr);
    }

    std::string get_path(const std::string &fldr, const std::string &file) {
        std::string tmp{fldr};
        if (fldr[fldr.length() - 1] != '/') tmp += '/';
        std::cout << "\"" << tmp + file << "\"\n";
        return tmp + file;
    }

    void recieve_bytes(char *buffer, int msg_sock, uint32_t expected) {
        // TODO 0
        // if (expected == 0) {
        //     printf("recv 0 bytes, error.\n");
        //     return false;
        // }

        ssize_t bytes = recv(msg_sock, buffer, expected, MSG_WAITALL);

        if (bytes < expected) {
            std::cout << __LINE__ << " " << __FILE__ << "\n";
            throw excpt::file_excpt(std::strerror(errno));
        }
    }

    void send_bytes(const char *buffer, int msg_sock, uint32_t expected) {
        errno = 0;
        ssize_t bytes = send(msg_sock, buffer, expected, MSG_NOSIGNAL);

        if (errno != 0 || bytes < expected) {
            std::cout << __LINE__ << " " << __FILE__ << "\n";
            throw excpt::file_excpt(std::strerror(errno));
        }

    }

    // TODO throw czy co jak coś się nie powiedzie?
    void receive_file(std::string path, size_t f_size, int sock) {
        FILE *fp = fopen(path.c_str(), "w+b"); // file existed
        if (!fp) {
            std::cout << __LINE__ << " " << __FILE__ << "\n";
            throw excpt::file_excpt(std::strerror(errno));
        }

        char buffer[BUFFER_SIZE];

        uint64_t recieved = 0;
        for (recieved = BUFFER_SIZE; recieved < f_size; recieved += BUFFER_SIZE) {
            recieve_bytes(buffer, sock, BUFFER_SIZE);

            if (fwrite(buffer, 1, BUFFER_SIZE, fp) != BUFFER_SIZE) {
                std::cout << __LINE__ << " " << __FILE__ << "\n";
                throw excpt::file_excpt(std::strerror(errno));
            }
        }
        recieved -= BUFFER_SIZE;
        recieve_bytes(buffer, sock, f_size - recieved);

        if (fwrite(buffer, 1, f_size - recieved, fp) != f_size - recieved) {
            fclose(fp);
            std::cout << __LINE__ << " " << __FILE__ << "\n";
            throw excpt::file_excpt(std::strerror(errno));
        }

        if (fclose(fp) == EOF) {
            std::cout << __LINE__ << " " << __FILE__ << "\n";
            throw excpt::file_excpt(std::strerror(errno));
        }

        printf("Recieved all.\n");
    }

    void send_file(std::string path, size_t f_size, int sock) {
        FILE *fp = fopen(path.c_str(), "rb");
        if (!fp) {
            std::cout << __LINE__ << " " << __FILE__ << "\n";
            throw excpt::file_excpt(std::strerror(errno));
        }

        int sent;
        char buffer[BUFFER_SIZE];

        for (sent = BUFFER_SIZE; sent < f_size; sent += BUFFER_SIZE) {
            if (fread(buffer, 1, BUFFER_SIZE, fp) != BUFFER_SIZE) {
                fclose(fp);
                std::cout << __LINE__ << " " << __FILE__ << "\n";
                throw excpt::file_excpt(std::strerror(errno));
            }
            send_bytes(buffer, sock, BUFFER_SIZE);
        }
        sent -= BUFFER_SIZE;

        if (fread(buffer, 1, f_size - sent, fp) != f_size - sent) {
            fclose(fp);
            std::cout << __LINE__ << " " << __FILE__ << "\n";
            throw excpt::file_excpt(std::strerror(errno));
        }
        send_bytes(buffer, sock, f_size - sent);

        printf("Sent requested fragment.\n");

        if (fclose(fp) == EOF) {
            std::cout << __LINE__ << " " << __FILE__ << "\n";
            throw excpt::file_excpt(std::strerror(errno));
        }
    }
}
#endif //COMMON_HPP
