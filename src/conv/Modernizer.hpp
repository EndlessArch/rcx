#ifndef RCX_CONV_MODERNIZER_HPP
#define RCX_CONV_MODERNIZER_HPP

#include "1:Parse/CTX/Context.hpp"
#define NSRCXBGN \
namespace rcx {

#define NSRCXEND \
} // ns rcx

#include <functional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Optional.h>

#include <spdlog/spdlog.h>

NSRCXBGN

namespace {

template <typename T, typename... Args>
struct has : std::false_type {};

template <typename T, typename... Args>
struct has<T, std::variant<T, Args...>> : std::true_type {};

template <typename T, typename U, typename... Args>
struct has<T, std::variant<U, Args...>> : has<T, std::variant<Args...>> {};

template <typename T, typename V>
constexpr bool does_variant_have_v = has<T, V>::value;

enum class EPackageStatus {
    Success,
    Failed
};

using BrokenPackage = struct __brkn_pkg {
    using callback_t = typename std::function<llvm::StringRef(void)>;

    std::variant<llvm::StringRef, callback_t> cont_;

    struct __brkn_pkg
    setErrPrtCB(llvm::StringRef sr) noexcept {
        this->cont_ = sr;
        return *this;
    }

    struct __brkn_pkg
    setErrPrtCB(callback_t && cb) noexcept {
        this->cont_ = std::move(cb);
        return *this;
    }

    llvm::StringRef operator()(void) noexcept {
        return std::visit([](auto && rhs) -> llvm::StringRef {
            if constexpr (std::is_same_v<decltype(rhs), callback_t>)
                return std::get<callback_t>(rhs)();
            
            if constexpr (std::is_same_v<decltype(rhs), llvm::StringRef>)
                return std::get<llvm::StringRef>(rhs);
            
            return "\xd";
        }, cont_);
    }
};

// xand

template <bool, bool>
struct xand : std::false_type {};

template <>
struct xand<true, true> : std::true_type {};

template <>
struct xand<false, false> : std::true_type {};

template <typename T, typename U>
inline constexpr bool xand_v = xand<T::value, U::value>::value;

// is_variant 

template <typename... Args>
struct is_variant : std::false_type {};

template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {};

template <typename... Args>
inline constexpr bool is_variant_v = is_variant<Args...>::value;

template <typename T, typename... Args>
constexpr auto bind_constructor(Args&&... args) noexcept {
    // IDK how to std::bind template constructor
    return [&](Args&&...) -> T { return T(std::forward<Args>(args)...); };
}

} // ns anon

template <typename T,
    typename FallbackConstructor>
struct _Package {
    using __fbc_r_t = typename std::invoke_result_t<FallbackConstructor>;

    static_assert(
    std::disjunction_v<
        std::is_convertible<__fbc_r_t, T>,
        std::is_nothrow_constructible<__fbc_r_t, T>
    >, "Fallback constructor's return type should be construct/convertible to type `T` of Package<>");

    std::variant<T, BrokenPackage> package_content_;

    FallbackConstructor fb_;

    template <typename _T, typename... Args>
    _Package(_T && val, Args&&... cargs)
    : package_content_(std::forward<_T>(val)), fb_(bind_constructor<T>(cargs...)) {
        static_assert(std::is_constructible_v<T, Args...>,
        "Unable to build fallback constructor");
    }

    template <typename _T, typename _FB>
    _Package(_T && val, _FB && fb)
    : package_content_(std::forward<_T>(val)), fb_(std::forward<_FB>(fb)) {}

    template <typename _T, typename... Args>
    static _Package<T, FallbackConstructor>
    makeBroken(_T && err, Args&&... fbcArgs) noexcept {
        static_assert(
            std::disjunction_v<
                std::is_constructible<_T, llvm::StringRef>,
                std::is_convertible<_T, llvm::StringRef>,
                std::is_invocable_r<llvm::StringRef, _T>
            >,  "'_T && err' should either be construct/convertible to `llvm::StringRef`"
                " or have `llvm::StringRef` as return type"
        );

        return _Package<T, FallbackConstructor>(
            BrokenPackage{}.setErrPrtCB(std::forward<_T>(err)),
            std::forward<Args>(fbcArgs)...);
    }

    T open() noexcept {
        return \
        std::visit([this](auto && arg) -> T {
            if constexpr (std::is_same_v<decltype(arg), T>)
                return std::get<T>(arg);
            
            if constexpr (std::is_same_v<decltype(arg), BrokenPackage>) {
                if(llvm::StringRef errMsg = std::get<BrokenPackage>(arg)(); errMsg == "\xd")
                    spdlog::error("Error while opening package"
                    "; broken package returned error message for nothing");
                else spdlog::warn(errMsg.str());
                
                return this->fb_();
            }

            spdlog::error(
            "Failed to open package; package has neither expected content nor error callback:"
            " replacing task by instantly default constructed");
            return this->fb_();
        }, this->package_content_);
    }

    inline auto operator()(void) noexcept { return this->open(); }
};

template <typename T, typename FBC = std::function<T()>>
using Package = struct _Package<T, FBC>;

NSRCXEND

#endif