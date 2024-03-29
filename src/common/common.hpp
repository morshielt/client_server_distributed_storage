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

    static const int32_t TTL_VALUE{64};
    static const int32_t MAX_UDP_PACKET_SIZE{65489};
    static const int32_t BUFFER_SIZE{512 * 1024};

    static const size_t CMD_SIZE = 10;
    static const int32_t SIMPL_CMD_SIZE{18};

    static const char SEP = '\n';

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
        return tmp + file;
    }

    void receive_bytes(char *buffer, int msg_sock, uint32_t expected) {
        ssize_t bytes = recv(msg_sock, buffer, expected, MSG_WAITALL);

        if (bytes < expected && errno != EINTR) {
            throw excpt::file_excpt(std::strerror(errno));
        }
    }

    void send_bytes(const char *buffer, int msg_sock, uint32_t expected) {
        errno = 0;
        ssize_t bytes = send(msg_sock, buffer, expected, MSG_NOSIGNAL);

        if ((errno != 0 || bytes < expected) && errno != EINTR) {
            throw excpt::file_excpt(std::strerror(errno));
        }
    }

    void receive_file(const std::string &out_fldr, const std::string &filename, size_t f_size, int sock) {
        FILE *fp = fopen(get_path(out_fldr, filename).c_str(), "w+b"); // file existed
        if (!fp) {
            throw excpt::file_excpt(std::strerror(errno));
        }

        char buffer[BUFFER_SIZE];

        uint64_t recieved = 0;
        for (recieved = BUFFER_SIZE; recieved < f_size; recieved += BUFFER_SIZE) {
            receive_bytes(buffer, sock, BUFFER_SIZE);

            if (fwrite(buffer, 1, BUFFER_SIZE, fp) != BUFFER_SIZE) {
                throw excpt::file_excpt(std::strerror(errno));
            }
        }
        recieved -= BUFFER_SIZE;
        receive_bytes(buffer, sock, f_size - recieved);

        if (fwrite(buffer, 1, f_size - recieved, fp) != f_size - recieved) {
            fclose(fp);
            throw excpt::file_excpt(std::strerror(errno));
        }

        if (fclose(fp) == EOF) {
            throw excpt::file_excpt(std::strerror(errno));
        }

    }

    void receive_file(const std::string &out_fldr, const std::string &filename, int sock) {
        FILE *fp = fopen(get_path(out_fldr, filename).c_str(), "w+b");
        if (!fp) {
            throw excpt::file_excpt(std::strerror(errno));
        }

        char buffer[BUFFER_SIZE];

        ssize_t recieved = 1;
        while (recieved != 0 && recieved != -1) {
            recieved = recv(sock, buffer, BUFFER_SIZE, 0);

            if (recieved < 0) {
                throw excpt::file_excpt(std::strerror(errno));
            }

            if (fwrite(buffer, 1, recieved, fp) != (size_t) recieved) {
                throw excpt::file_excpt(std::strerror(errno));
            }
        }

        if (fclose(fp) == EOF) {
            throw excpt::file_excpt(std::strerror(errno));
        }
    }

    void send_file(FILE *fp, size_t f_size, int sock) {
        uint32_t sent;
        char buffer[BUFFER_SIZE];

        for (sent = BUFFER_SIZE; sent < f_size; sent += BUFFER_SIZE) {
            if (fread(buffer, 1, BUFFER_SIZE, fp) != BUFFER_SIZE) {
                fclose(fp);
                throw excpt::file_excpt(std::strerror(errno));
            }
            send_bytes(buffer, sock, BUFFER_SIZE);
        }
        sent -= BUFFER_SIZE;

        if (fread(buffer, 1, f_size - sent, fp) != f_size - sent) {
            fclose(fp);
            throw excpt::file_excpt(std::strerror(errno));
        }
        send_bytes(buffer, sock, f_size - sent);

        if (fclose(fp) == EOF) {
            throw excpt::file_excpt(std::strerror(errno));
        }
    }

    void send_file(const std::string &out_fldr, const std::string &filename, size_t f_size, int sock) {
        FILE *fp = fopen(get_path(out_fldr, filename).c_str(), "rb");
        if (!fp) {
            throw excpt::file_excpt(std::strerror(errno));
        }

        send_file(fp, f_size, sock);
    }

}
#endif //COMMON_HPP
