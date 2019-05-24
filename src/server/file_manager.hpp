#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include "../common/validation.hpp"
#include "../common/sockets.hpp"
#include <map>
#include <set>
#include <mutex>
#include <shared_mutex>

namespace valid = sik_2::validation;
namespace sckt = sik_2::sockets;
namespace fs =  boost::filesystem;

namespace sik_2::file_manager {
    class file_manager {

    private:
        std::map<std::string, uint64_t> files{};
        std::set<std::string> uploading{};
        std::shared_mutex mutex_;

        void init() {
            fs::path path(shrd_fldr);

            try {
                if (exists(path)) {
                    if (is_directory(path)) {

                        for (fs::directory_entry &p: fs::directory_iterator(path)) {
                            if (is_regular_file(p)) {
                                std::cout << "REGULAR : " << p.path().filename().string() << '\n';
                                // XXX

                                files.insert({p.path().filename().string(), fs::file_size(p)});
                                free_space -= fs::file_size(p);
                                if (free_space < 0) {
                                    // TODO THROW
                                }
                            } else {
                                std::cout << "[sanity check] NOT : " << p << '\n';
                            }
                        }
                    } else {
                        throw std::invalid_argument(shrd_fldr + " is not a directory");
                    }
                } else {
                    throw std::invalid_argument(shrd_fldr + "does not exist");
                }
            }
            catch (const fs::filesystem_error &ex) {
                throw;
                // std::cout << ex.what() << '\n';
            }
        }

    public:
        file_manager(int64_t max_space, const std::string &shrd_fldr)
            : free_space{max_space}, max_space{max_space}, shrd_fldr{shrd_fldr} {
            // if (!valid::valid_directory(shrd_fldr)) {
            //     throw excpt::invalid_argument{"shrd_fldr = " + shrd_fldr};
            // }

            if (!valid::in_range_incl<int64_t>(max_space, 0, INT64_MAX)) {
                throw excpt::invalid_argument{"max_space = " + std::to_string(max_space)};
            }

            init();
        }

        uint64_t get_free_space() {
            std::shared_lock lock(mutex_);
            std::cout << "get_free_space : " << free_space << "\n";
            return (free_space > 0) ? free_space : 0;
        }

        bool filename_nontaken(const std::string &name) {
            return files.find(name) == files.end();
        }

        bool is_being_uploaded(const std::string &name) {
            return uploading.find(name) != uploading.end();
        }

        bool add_file(std::string name, uint32_t f_size) {
            std::unique_lock lock(mutex_);
            if (!name.empty() && name.find('/') == std::string::npos &&
                f_size < get_free_space() && filename_nontaken(name) && !is_being_uploaded(name)) {
                free_space -= f_size;
                uploading.insert(name);
                // files.insert({name, f_size});
                return true;
            }
            return false;
        }

        void save_file(sckt::socket_TCP_server &tcp_sock, std::string path, uint64_t f_size) {

            try {
                tcp_sock.get_connection();
                cmmn::receive_file(path, f_size, tcp_sock.get_sock());
                files.insert({std::string{path, path.find_last_of('/') + 1, path.length()}, f_size});
                uploading.erase(std::string{path, path.find_last_of('/') + 1, path.length()});
            }
            catch (excpt::file_excpt &e) {
                free_space += f_size;
                // TODO czemu tu było "\n"????
                uploading.erase(std::string{path, path.find_last_of('/') + 1, path.length()});

                if (remove(path.c_str()) != 0) {
                    // throw ? czy co
                    std::cout << __LINE__ << "\n";
                    std::cout << "MEH\n";
                }
            }

        }

        void remove_file(std::string path) {
            std::unique_lock lock(mutex_);
            std::cout << "djsdj :: " << std::string{path, path.find_last_of('/') + 1, path.length()} << "\n";
            std::string fn = std::string{path, path.find_last_of('/') + 1, path.length()};

            if (!filename_nontaken(fn)) {
                std::cout << "TAKEN\n";

                free_space += fs::file_size(path);
                std::cout << __LINE__ << "\n";
                files.erase(fn);
                std::cout << __LINE__ << "\n";
                if (remove(path.c_str()) != 0) {
                    // throw ? czy co
                    std::cout << __LINE__ << "\n";
                    std::cout << "MEH\n";
                }
            }
        }

        // TODO nie działą puste XDDDDD
        std::string get_files(const std::string &sub) {
            std::string tmp{};

            std::shared_lock lock(mutex_);

            for (auto &t : files) {
                if ((!sub.empty() && t.first.find(sub) != std::string::npos) || sub.empty()) {
                    tmp += t.first + cmmn::SEP;
                }
            }

            std::cout << "tmp::::::::::::: " << tmp << "\n";
            return tmp;
        }

        std::string cut_nicely(std::string &str) {
            std::string tmp{str, 0, std::min<size_t>(cmmn::MAX_UDP_PACKET_SIZE, str.length())};

            size_t last = tmp.find_last_of('\n');
            std::cout << "last " << last << "end: " << (tmp.length() - 1) << "\n";

            tmp = std::string{tmp, 0, last};
            str = std::string{str, last + 1, str.length()};

            return tmp;
        }

        bool send_file(sckt::socket_TCP_server &tcp_sock, std::string path) {
            try {
                tcp_sock.get_connection();

                std::shared_lock lock(mutex_);

                if (!filename_nontaken(std::string{path, path.find_last_of('/') + 1, path.length()})) {

                    FILE *fp = fopen(path.c_str(), "rb");
                    if (!fp) {
                        std::cout << __LINE__ << " " << __FILE__ << "\n";
                        throw excpt::file_excpt(std::strerror(errno));
                    }
                    cmmn::send_file(fp, fs::file_size(path), tcp_sock.get_sock());
                    std::cout << "ALL GOOOD\n";

                    return true;
                } else {
                    std::cout << "\"" << std::string{path, path.find_last_of('/') + 1, path.length()} << "\"\n";
                    return false;
                }
            } catch (excpt::excpt_with_msg &e) {
                std::cout << "NO WYSYŁANIE SIĘ NIE UDAŁO\n";
                return false;
            }
        }

    private:
        int64_t free_space{};
        int64_t max_space{};
        std::string shrd_fldr{};

    };
};

#endif //FILE_MANAGER_HPP
