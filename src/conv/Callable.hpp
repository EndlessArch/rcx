#ifndef RCX_CONV_CALLABLE_HPP
#define RCX_CONV_CALLABLE_HPP

#include <conv/Modernizer.hpp>

NSRCXBGN

namespace callable {

// NOTE: struct `closure_traits` works for closure with single `operator()` method
// in other words, this wont work for methods with template/auto-typed arguments
// since they are assumed to be overloaded, multiple candidates
// it's impossible for compiler to refer an address
// template <typename>
// struct closure_traits {
//     using return_t = void;
//     using arg_ts   = void;
//     static constexpr std::size_t arity_v = 0;
//     static constexpr bool is_callable_v  = false;
// };

template <typename T, typename = void>
struct closure_traits : closure_traits<decltype(&T::operator())> {}; // <<<<

template <typename C, typename R, typename... Args>
struct closure_traits<R (C::*)(Args...) const> {
    using return_t = R;
    using arg_ts   = std::tuple<Args...>;
    static constexpr std::size_t arity_v = sizeof...(Args);
    static constexpr bool is_callable_v  =
        std::is_invocable<R (C::*)(Args...), R, Args...>::value;
};

// TODO: find the way that works with overloaded `operator()`s than `closure_traits`
// TODO: update concept `CallableObj` with it

// template <typename T, class... Args>
// concept CallableObj =
//     std::is_class_v<std::remove_cvref_t<T>> &&
//     requires(std::remove_cvref_t<T> t, Args... args) { t(args...); };

//     // requires(std::remove_cvref_t<T> t) { closure_traits<decltype(t)>::is_callable_v; }

} // ns Callable

template <typename T>
concept Iterable = requires(T& t) { std::begin(t); std::end(t); };

NSRCXEND

#endif // RCX_CONV_CALLABLE_HPP