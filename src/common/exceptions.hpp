#include <utility>

#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP
namespace sik_2::exceptions {

    class excpt_with_msg : public std::exception {

    private:
        std::string msg;

    public:
        explicit excpt_with_msg(std::string &&msg) : msg{std::move(msg)} {}

        const char *what() const noexcept override {
            return msg.c_str();
        }
    };

    class invalid_argument : public excpt_with_msg {

    public:
        // działa tylko z rvalue, jakby co usunąć &&
        explicit invalid_argument(std::string &&msg) : excpt_with_msg{"invalid argument " + std::move(msg)} {}
    };

    class socket_excpt : public excpt_with_msg {

    public:
        // działa tylko z rvalue, jakby co usunąć &&
        explicit socket_excpt(std::string &&msg) : excpt_with_msg{"socket exception " + std::move(msg)} {}
    };

    class file_excpt : public excpt_with_msg {

    public:
        // działa tylko z rvalue, jakby co usunąć &&
        explicit file_excpt(std::string &&msg) : excpt_with_msg{"file exception " + std::move(msg)} {}
    };


    // class Invalid_ip : public std::exception {
    // public:
    //     const char *what() const noexcept override {
    //         return "invalid IP address";
    //     }
    // };
    //
    // class Invalid_dir : public std::exception {
    // public:
    //     const char *what() const noexcept override {
    //         return "invalid directory";
    //     }
    // };

    // class invalid_timeout : public std::exception {
    // public:
    //     const char *what() const noexcept override {
    //         return "invalid timeout value";
    //     }
    // };

}
#endif //EXCEPTIONS_HPP
