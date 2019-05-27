#include <utility>

#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP
namespace sik_2::exceptions {

    class excpt_with_msg : public std::exception {

    public:
        explicit excpt_with_msg(std::string &&msg) : msg{std::move(msg)} {}

        const char *what() const noexcept override {
            return msg.c_str();
        }

    private:
        std::string msg;
    };

    class invalid_argument : public excpt_with_msg {

    public:
        explicit invalid_argument(std::string &&msg) : excpt_with_msg{"invalid argument " + std::move(msg)} {}
    };

    class socket_excpt : public excpt_with_msg {

    public:
        explicit socket_excpt(std::string &&msg) : excpt_with_msg{"socket exception " + std::move(msg)} {}
    };

    class file_excpt : public excpt_with_msg {

    public:
        explicit file_excpt(std::string &&msg) : excpt_with_msg{"file exception " + std::move(msg)} {}
    };
}
#endif //EXCEPTIONS_HPP
