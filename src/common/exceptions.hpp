#include <utility>

#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP
namespace sik_2::exceptions {

    class Excpt_with_msg : public std::exception {
    private:
        std::string msg;

    public:
        explicit Excpt_with_msg(std::string &&msg) : msg{std::move(msg)} {}

        const char *what() const noexcept override {
            return msg.c_str();
        }
    };

    class Invalid_param : public Excpt_with_msg {
    public:
        // działa tylko z rvalue, jakby co usunąć &&
        explicit Invalid_param(std::string &&msg) : Excpt_with_msg{"invalid parameter " + std::move(msg)} {}
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
