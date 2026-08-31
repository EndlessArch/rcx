#ifndef RCX_CONV_TUPLES_HPP
#define RCX_CONV_TUPLES_HPP

#include <tuple>
#include <type_traits>

#include <conv/Modernizer.hpp>

NSRCXBGN

namespace tupl {

namespace {

// template <typename T, std::size_t I, typename... _U>
// struct cnt : std::false_type {};

// template <typename T, std::size_t I, typename... _U>
// struct cnt<T, I, T, _U...> : std::true_type {
//     // using N = I;
// };

// template <typename T, std::size_t I, typename U, typename... _U>
// struct cnt<T, I, U, _U...> : cnt<T, 1+I, _U...> {};

// find_tuple
template <int I, typename... Ts>
constexpr int __find_tupl(std::tuple<Ts...>) noexcept {
    return -1;
}

template <int I, typename T, typename U, typename... _U>
constexpr int __find_tupl(std::tuple<T, T, _U...>) noexcept {
    return I;
}

template <int I, typename T, typename U, typename... _U>
constexpr int __find_tupl(std::tuple<T, U, _U...>) noexcept {
    return __find_tupl<1+I, T, _U...>();
}

// is_tuple
template <typename...>
struct __is_tupl : std::false_type {};

template <typename... Ts>
struct __is_tupl<std::tuple<Ts...>> : std::true_type {};

// is_tuple + candidates support
template <typename... Ts>
struct __is_tupl<rcx::candidates<Ts...>>
    : std::disjunction<__is_tupl<Ts>...> {};

// has_tupl
template <typename...>
struct __has_tupl : std::false_type {};

template <typename T, typename U, typename... Us>
struct __has_tupl<T, std::tuple<U, Us...>> : __has_tupl<T, std::tuple<Us...>> {};

template <typename T, typename... Us>
struct __has_tupl<T, std::tuple<T, Us...>> : std::true_type {};

// has_tuple + candidates support
template <typename... Cs, typename... Ts>
struct __has_tupl<rcx::candidates<Cs...>, std::tuple<Ts...>>
    : std::disjunction<__has_tupl<Cs, std::tuple<Ts...>>...> {};

// // is_pair_first
// template <typename...>
// struct __is_pair_first : std::false_type {};

// template <typename T, typename U>
// struct __is_pair_first<T, std::pair<T, U>> : std::true_type {};

// template <typename...>
// struct __is_pair_sec : std::false_type {};

// template <typename T, typename U>
// struct __is_pair_sec<T, std::pair<U, T>> : std::true_type {};
} // ns anon

template <typename T, typename... _U>
constexpr int find_tupl(std::tuple<_U...> t) noexcept {
    return __find_tupl<0, T, _U...>(t);
}

// template <typename T, typename U>
// constexpr bool has_tupl(U&&) noexcept {
//     return false; // not a tuple
// }

// template <typename T, typename... _U>
// constexpr bool has_tupl(std::tuple<_U...>) noexcept {
//     return cnt<T, 0, _U...>::value;
// }

template <typename T, typename U>
static constexpr
bool has_tupl_v = __has_tupl<T, U>::value;

// template <typename T>
// constexpr bool is_tupl(T) noexcept {
//     return __is_tupl<T>::value;
// }

template <typename T>
static constexpr
bool is_tupl_v = __is_tupl<T>::value;

// is_pair
template <typename...>
struct is_pair : std::false_type {};

template <typename A, typename B>
struct is_pair<std::pair<A, B>> : std::true_type {};

template <typename T>
constexpr bool is_pair_v = is_pair<T>::value;

// has_pair
template <typename...>
struct has_pair : std::false_type {};

template <typename T, typename P>
struct has_pair<T, std::pair<T, P>> : std::true_type {};

template <typename T, typename P>
struct has_pair<T, std::pair<P, T>> : std::true_type {};

template <typename T, typename P, typename... Cs>
struct has_pair<rcx::candidates<Cs...>, std::pair<T, P>>
    : std::disjunction<has_pair<Cs, std::pair<T, P>>...> {};

template <typename T, typename P>
constexpr bool has_pair_v = has_pair<T, P>::value;

static_assert(has_pair_v<rcx::candidates<std::string, char>, std::pair<parser::Token, char>>);

// // is_pair_N
// template <typename T, typename P>
// constexpr bool is_pair_first_v = __is_pair_first<T, P>::value;

// template <typename T, typename P>
// constexpr bool is_pair_second_v = __is_pair_sec<T, P>::value;

template <typename...>
struct get_other {
    using type = void;
};

template <typename T, typename P>
struct get_other<T, std::pair<T, P>> {
    using type = P;
};

template <typename T, typename P>
struct get_other<T, std::pair<P, T>> : get_other<T, std::pair<T, P>> {};

template <typename T, typename P, typename = std::enable_if_t<is_pair_v<P>>>
using get_other_t = get_other<T, P>::type;

} // ns tupl

NSRCXEND

#endif // RCX_CONV_TUPLES_HPP