#ifndef RCX_CONV_LAZY_HPP
#define RCX_CONV_LAZY_HPP

#include <conv/Modernizer.hpp>

NSRCXBGN

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