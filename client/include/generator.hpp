#ifndef GENERATOR_H_
#define GENERATOR_H_

#include <array>
#include <functional>
#include <iostream>
#include <type_traits>

#include "nlohmann/json.hpp"

#include "socket.hpp"

namespace mrpc {
using json = nlohmann::json;

// type list for arguments
template<typename... Args>
struct type_list;

template<typename T>
struct type_list<T> { using head = T; };

template<typename Head, typename... Tail>
struct type_list<Head, Tail...> : type_list<Head> {
    using tail = type_list<Tail...>;
};

// calculate size of type list
template<typename... Args>
struct count;

template<typename... Args>
struct count<type_list<Args...>>
    : std::integral_constant<std::size_t, sizeof...(Args)> {};

// check whether it has tail
template<typename ArgTypeList>
struct has_tail : std::conditional_t<(count<ArgTypeList>::value > 1),
                                     std::true_type, std::false_type> {};

template<typename T>
void to_json(json& arg_json, const T& arg) {
    arg_json = arg;
}

// todo: try to use concept to make overload more precise
template<typename T>
void add_type(json& j, T* const arg) {
    j["type"] = "pointer";
    json arg_json;
    to_json(arg_json, *arg);
    j["arg"] = arg_json;
}

// todo: try rvalue reference
template<typename T>
void add_type(json& j, const T& arg) {
    j["type"] = "object";
    json arg_json;
    to_json(arg_json, arg);
    j["arg"] = arg_json;
}

template<typename T>
void add_type(json& j, std::unique_ptr<T>& arg) {
    j["type"] = "unique pointer";
    json arg_json;
    to_json(arg_json, *arg);
    j["arg"] = arg_json;
}

template<typename T>
void add_type(json& j, std::shared_ptr<T>& arg) {
    j["type"] = "shared pointer";
    json arg_json;
    to_json(arg_json, *arg);
    j["arg"] = arg_json;
}

// Arguments Parser
template<typename... Args>
class ArgsParser {
    template<typename T>
    static void parse(json& args_array, size_t index, T& arg) {
        json type_arg;
        add_type(type_arg, arg);
        args_array[index] = type_arg;
    };

    template<typename ArgTypeList, bool HasTail, typename... ArgList>
    struct args_parser_impl;

    // Case 1: has tail
    template<typename ArgTypeList, typename Head, typename... Tail>
    struct args_parser_impl<ArgTypeList, true, Head, Tail...> {
        static void call(json& args_array, size_t index, Head& head, Tail&... tail) {
            parse(args_array, index, head);

            using ArgTypeListTail = typename ArgTypeList::tail;

            constexpr bool has_tail_v = has_tail<ArgTypeListTail>::value;

            args_parser_impl<ArgTypeListTail, has_tail_v, Tail...>::call(args_array, index + 1, tail...);
        }
    };

    // Case 2: no tail, not empty
    template<typename ArgTypeList, typename Head>
    struct args_parser_impl<ArgTypeList, false, Head> {
        static void call(json& args_array, size_t index, Head& head) {
            parse(args_array, index, head);
        }
    };

public:
    static void parser(json& serialization, const std::string& func_name, Args&... args) {
        using ArgTypeList = type_list<Args...>;

        constexpr bool has_tail_v = has_tail<ArgTypeList>::value;
        constexpr size_t args_len = count<ArgTypeList>::value;

        json args_array;

        serialization["func_name"] = func_name;
        serialization["length"] = args_len;

        if (args_len > 0)
            args_parser_impl<ArgTypeList, has_tail_v, Args...>::call(args_array, 0, args...);

        serialization["args"] = args_array;
    }
};

// todo: handle non-const reference or pointer parameter
// todo: after finishing unit testing, support perfect forward
// todo: change return type from string to Reply after implementing server, and remove relate test
template<typename Reply, typename... Args>
auto wrapper(const std::string& func_name) {
    auto new_func = [func_name](Args&&... args) {
        json serialization{};
        ArgsParser<Args...>::parser(serialization, func_name, args...);
        std::string json_str = nlohmann::to_string(serialization);
        std::size_t json_len = json_str.length();

        auto host_socket = std::make_unique<mrpc::Socket>(AF_INET, SOCK_STREAM, 0, 10);

        host_socket->send_length(json_len, 0);
        host_socket->sendn(json_str, 0);

        std::int64_t recv_size = 0;
        host_socket->recv_length(host_socket->get_socket(), (std::size_t*)&recv_size, 0);
        if (recv_size < 0) {
            std::cerr << "error" << std::endl;
            exit(1);
        }

        std::string recv_data(recv_size, 0);
        host_socket->recvn(host_socket->get_socket(), recv_data, 0);
        std::string ret_str{recv_data};

        json ret = json::parse(ret_str);

        Reply reply{ret["value"]};
        return reply;
    };
    return new_func;
}
}// namespace mrpc

#endif// GENERATOR_H_
