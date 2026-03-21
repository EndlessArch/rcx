#ifndef RCX_CONV_TUPLES_HPP
#define RCX_CONV_TUPLES_HPP

#include <tuple>

#include <conv/Modernizer.hpp>

NSRCXBGN

namespace tupl {

namespace {

template <typename T, std::size_t I, typename... _U>
struct cnt : std::false_type {};

template <typename T, std::size_t I, typename... _U>
struct cnt<T, I, T, _U...> : std::true_type {
    // using N = I;
};

template <typename T, std::size_t I, typename U, typename... _U>
struct cnt<T, I, U, _U...> : cnt<T, 1+I, _U...> {};

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

// is_pair
template <typename...>
struct __is_pair : std::false_type {};

template <typename A, typename B>
struct __is_pair<std::pair<A, B>> : std::true_type {};

// is_pair_first
template <typename...>
struct __is_pair_first : std::false_type {};

template <typename T, typename U>
struct __is_pair_first<T, std::pair<T, U>> : std::true_type {};

template <typename...>
struct __is_pair_sec : std::false_type {};

template <typename T, typename U>
struct __is_pair_sec<T, std::pair<U, T>> : std::true_type {};
} // ns anon

template <typename T, typename... _U>
constexpr int find_tupl(std::tuple<_U...> t) noexcept {
    return __find_tupl<0, T, _U...>(t);
}

template <typename T, typename... _U>
constexpr bool has_tupl(std::tuple<_U...>) noexcept {
    return cnt<T, 0, _U...>::value;
}

template <typename T>
constexpr bool is_tupl(T) noexcept {
    return __is_tupl<T>::value;
}

template <typename T>
constexpr bool is_pair_v = __is_pair<T>::value;

template <typename T, typename P>
constexpr bool is_pair_first_v = __is_pair_first<T, P>::value;

template <typename T, typename P>
constexpr bool is_pair_second_v = __is_pair_sec<T, P>::value;

} // ns tupl

NSRCXEND

#endif // RCX_CONV_TUPLES_HPP