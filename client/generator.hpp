#ifndef GENERATOR_H_
#define GENERATOR_H_

#include <functional>
#include <type_traits>

namespace mrpc {
// type list for arguments
template <typename... Args> struct type_list;

template <typename T> struct type_list<T> { using head = T; };

template <typename Head, typename... Tail>
struct type_list<Head, Tail...> : type_list<Head> {
    using tail = type_list<Tail...>;
};

// calculate size of type list
template <typename... Args> struct count;

template <typename... Args>
struct count<type_list<Args...>>
    : std::integral_constant<std::size_t, sizeof...(Args)> {};

// check wheather has tail
template <typename ArgTypeList>
struct has_tail : std::conditional_t<(count<ArgTypeList>::value > 1),
                                     std::true_type, std::false_type> {};

// check wheather empty list
template <typename ArgTypeList>
struct empty_list : std::conditional_t<(count<ArgTypeList>::value == 0),
                                       std::true_type, std::false_type> {};

// Handler
template <typename... Args>
class Handler {
    static void actual_for_void(){};
    template <typename T> static void actual_for_args(T &args){};

    template <typename ArgTypeList, bool HasTail, bool Empty, typename... ArgList>
    struct handle_impl;

    // Case 1: has tail
    template <typename ArgTypeList, typename Head, typename... Tail>
    struct handle_impl<ArgTypeList, true, false, Head, Tail...> {
        static void call(Head &head, Tail &... tail) {
            actual_for_args(head);

            using ArgTypeListTail = typename ArgTypeList::tail;

            constexpr bool has_tail_v = has_tail<ArgTypeListTail>::value;
            constexpr bool empty_list_v = empty_list<ArgTypeListTail>::value;

            handle_impl<ArgTypeListTail, has_tail_v, empty_list_v, Tail...>::call(tail...);
        }
    };

    // Case 2: no tail, not empty
    template <typename ArgTypeList, typename Head>
    struct handle_impl<ArgTypeList, false, false, Head> {
        static void call(Head &head) { actual_for_args(head); }
    };

    // Case 3: empty
    template <typename ArgTypeList> struct handle_impl<ArgTypeList, false, true> {
        static void call() { actual_for_void(); }
    };

public:
    static void handle(Args &... args) {
        using ArgTypeList = type_list<Args...>;

        constexpr bool has_tail_v = has_tail<ArgTypeList>::value;
        constexpr bool empty_list_v = empty_list<ArgTypeList>::value;

        handle_impl<ArgTypeList, has_tail_v, empty_list_v, Args...>::call(args...);
    }
};

template <typename Reply, typename... Args>
auto wrapper() {
    auto new_func = [](Args&&... args) {
        Handler<Args...>::handle(args...);
        Reply reply = 0;
        return reply;
    };
    return new_func;
}
} // namespace mrpc

#endif // GENERATOR_H_
