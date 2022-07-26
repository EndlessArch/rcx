#ifndef RCX_CONV_MODERNIZER_HPP
#define RCX_CONV_MODERNIZER_HPP

#include <string>
#include <vector>
#define NSRCXBGN \
namespace rcx {

#define NSRCXEND \
} // ns rcx

#include <functional>
#include <variant>
#include <type_traits>

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

template <bool, bool>
struct xand : std::false_type {};

template <>
struct xand<true, true> : std::true_type {};

template <>
struct xand<false, false> : std::true_type {};

} // ns anon

template <typename T,
    typename FallbackConstructor = std::function<T()>>
struct Package {
    using checker_t
    = std::enable_if_t<
        std::conjunction_v<
            std::is_same<std::invoke_result_t<FallbackConstructor>, T>>,
            xand<std::is_nothrow_constructible_v<T>, std::is_same_v<FallbackConstructor, std::function<T()>>>>;

    std::variant<BrokenPackage, T> package_content_;

    FallbackConstructor fb_;

    template <typename _T>
    Package(_T && rhs) : package_content_(std::forward<_T>(rhs)) {}

    template <typename _T, typename _FB>
    Package(_T && rhs, _FB && fb) : package_content_(std::forward<_T>(rhs)), fb_(std::forward<_FB>(fb)) {}

    template <typename _T>
    static std::enable_if_t<std::disjunction_v<
        std::is_constructible<_T, llvm::StringRef>,
        std::is_convertible<_T, llvm::StringRef>,
        std::is_invocable_r<llvm::StringRef, _T>
    >, Package<T>>
    makeBroken(_T && err) noexcept {
        return Package<T>(BrokenPackage{}.setErrPrtCB(std::forward<_T>(err)));
    }

    T open() noexcept {
        return \
        std::visit([this](auto && arg) -> T {
            if constexpr (std::is_same_v<decltype(arg), T>)
                return std::get<T>(arg);
            
            if constexpr (std::is_same_v<decltype(arg), BrokenPackage>) {
                if(llvm::StringRef errMsg = std::get<BrokenPackage>(arg)(); errMsg == "\xd")
                    spdlog::error("Error while opening package; broken package returned error message for nothing");
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

NSRCXEND

#endif