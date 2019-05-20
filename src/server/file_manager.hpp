#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include "../common/validation.hpp"
#include <map>
// TODO mutex?
namespace valid = sik_2::validation;
namespace fs =  boost::filesystem;
namespace sik_2::file_manager {
    class file_manager {
    private:
        std::map<std::string, uint64_t> files{};
        int64_t free_space;
        int64_t max_space;
        std::string shrd_fldr;

        void init() {
            fs::path path(shrd_fldr);

            try {
                if (exists(path)) {
                    if (is_directory(path)) {
                        std::cout << path << " is a directory containing:\n";

                        for (fs::directory_entry &p: fs::directory_iterator(path)) {
                            if (is_regular_file(p)) {
                                std::cout << "REGULAR : " << p.path().filename().string() << '\n';
                                files.insert({p.path().filename().string(), fs::file_size(p)});
                                free_space -= fs::file_size(p);
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
            : max_space{max_space}, free_space{max_space}, shrd_fldr{shrd_fldr} {
            // if (!valid::valid_directory(shrd_fldr)) {
            //     throw excpt::Invalid_argument{"shrd_fldr = " + shrd_fldr};
            // }

            if (!valid::in_range_incl<int64_t>(max_space, 0, INT64_MAX)) {
                throw excpt::Invalid_argument{"max_space = " + std::to_string(max_space)};
            }

            init();
        }

        uint64_t get_free_space() {
            std::cout << "get_free_space : " << free_space << "\n";
            return free_space;
        }

        bool filename_nontaken(std::string name) {
            return files.find(name) == files.end();
        }
    };

};

#endif //FILE_MANAGER_HPP
