#ifndef RCX_CONV_LAZY_HPP
#define RCX_CONV_LAZY_HPP

#include <ranges>
#include <conv/Modernizer.hpp>

NSRCXBGN

namespace lazy {

namespace {

// template <typename T, std::ranges::input_range R>
// T to(R&& r) {
//     return T(std::begin(r), std::end(r));
// }

// template <typename C>
// struct to_closure {
//     template <std::ranges::input_range R>
//     C operator()(R&& r) const {
//         C c;
//         if constexpr (requires{ c.reserve(std::ranges::size(r)); }) {
//             c.reserve(std::ranges::size(r));
//         }
//         std::ranges::copy(r, std::back_inserter(c));
//         // return to<C>(std::forward<R>(r));
//         return c;
//     }
// };

struct to_vector_t {
    template <std::ranges::range R>
    auto operator()(R&& r) const {
        using T = std::ranges::range_value_t<R>;
        std::vector<T> result;
        std::ranges::copy(r, std::back_inserter(result));
        return result;
    }

    template <std::ranges::range R>
    friend auto operator|(R&& r, const to_vector_t& self) {
        return self(std::forward<R>(r));
    }
};

} // ns anon

// template <class C>
// inline constexpr to_closure<C> to{};

inline constexpr to_vector_t to_vector;

} // ns lazy

template <typename T, typename... _T>
class Lazy {
    std::function<T(_T...)> callback_ctr;
    std::optional<T> t_;
public:
    // Lazy(_T... args) : callback_ctr{[&t_](_T... _args){ return T(_args...); }} {}

    void setArgs(_T... args) noexcept {
        callback_ctr = [](_T... _args) noexcept {
            return T(_args...);
        };
    }

    T& operator*() & noexcept {
        if(t_.empty()) t_ = callback_ctr();
        return t_;
    }
};

NSRCXEND

#endif // RCX_CONV_LAZY_HPP