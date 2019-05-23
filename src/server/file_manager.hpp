#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include "../common/validation.hpp"
#include <map>
// TODO mutex?
// f_lock? idkkkkkkkkkkkkkkkkkkkkk

namespace valid = sik_2::validation;
namespace fs =  boost::filesystem;
namespace sik_2::file_manager {
    class file_manager {

    private:
        std::map<std::string, uint64_t> files{};
        // int64_t free_space;
        // int64_t max_space;
        // std::string shrd_fldr;

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
            std::cout << "get_free_space : " << free_space << "\n";
            return (free_space > 0) ? free_space : 0;
        }

        bool filename_nontaken(const std::string &name) {
            return files.find(name) == files.end();
        }

        bool add_file(std::string name, uint32_t f_size) {
            if (!name.empty() && name.find('/') == std::string::npos &&
                f_size < get_free_space() && filename_nontaken(name)) {
                free_space -= f_size;
                files.insert({name, f_size});
                return true;
            }
            return false;
        }

        void save_file(int sock, std::string path, uint64_t f_size) {

            try {
                cmmn::receive_file(path, f_size, sock);
            }
            catch (excpt::file_excpt &e) {
                std::cout << "DUPA BLADA\n";
                free_space += f_size;
                // TODO czemu tu było "\n"????
                files.erase(std::string{path, path.find_last_of('/'), path.length()});

                if (remove(path.c_str()) != 0) {
                    // throw ? czy co
                    std::cout << __LINE__ << "\n";
                    std::cout << "MEH\n";
                }
            }
        }

        void remove_file(std::string path) {
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

        void send_file(std::string path, int sock) {
            try {
                cmmn::send_file(path, fs::file_size(path), sock);
            } catch (excpt::file_excpt &e) {
                std::cout << "NO WYSYŁANIE SIĘ NIE UDAŁO\n";
            }
        }

    private:
        int64_t free_space{};
        int64_t max_space{};
        std::string shrd_fldr{};

    };
};

#endif //FILE_MANAGER_HPP
