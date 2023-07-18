#ifndef RCX_CONV_MODERNIZER_HPP
#define RCX_CONV_MODERNIZER_HPP

#include <condition_variable>
#include <optional>
#include <variant>

#define NSRCXBGN \
namespace rcx {

#define NSRCXEND \
} // ns rcx

NSRCXBGN

template <typename T, typename... Args>
struct has : std::false_type {};

template <typename T, typename... Args>
struct has<T, std::variant<T, Args...>> : std::true_type {};

template <typename T, typename U, typename... Args>
struct has<T, std::variant<U, Args...>> : has<T, std::variant<Args...>> {};

template <typename T, typename V>
constexpr bool does_variant_have_v = has<T, V>::value;

//

template <typename... As, typename... Bs>
constexpr std::variant<As..., Bs...>
__merge_variant_impl(std::variant<As...>, std::variant<Bs...>) noexcept {
    return std::declval<std::variant<As..., Bs...>>();
}

template <typename... As, typename... Bs>
constexpr auto
merge_variant_t(std::variant<As...> l, std::variant<Bs...> r) noexcept
-> std::variant<As..., Bs...> {
    return __merge_variant_impl(l, r);
}

//

template <template <class, class> typename T, typename A, typename B>
constexpr auto fill_every_case_impl() noexcept {
    return std::variant<T<A, B>, T<B, A>>{};
}

template <template <class, class> typename T, typename A, typename B,
typename C /* prevent ambiguousness */, typename... D>
constexpr auto fill_every_case_impl() noexcept {
    return \
    merge_variant_t(
        std::variant<T<A, B>, T<B, A>>{},
        fill_every_case_impl<T, A, C, D...>(),
        fill_every_case_impl<T, B, C, D...>());
}

template <template <class, class> typename T, typename... Args>
using fill_every_case = decltype(fill_every_case_impl<T, Args...>());

NSRCXEND

#include <parse/CTX/Context.hpp>

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

#define __BRKN_PKG_NULL_STRING "\xd"

using BrokenPackage = struct __brkn_pkg {
    using callback_t = typename std::function<llvm::StringRef(void)>;

    // either (error) message or message constructor
    std::variant<llvm::StringRef, callback_t> cont_;

    template <typename T, typename = std::enable_if_t<std::is_convertible_v<T, llvm::StringRef>>>
    struct __brkn_pkg
    setErrPrtCB(T && sr) noexcept {
        using t_t = decltype(sr);
        using sr_t = typename std::conditional_t<
            std::is_reference_v<t_t>,
            std::conditional_t< // TODO: is this part necessary?
                std::is_lvalue_reference_v<t_t>,
                t_t&,
                t_t&&>, t_t>;

        this->cont_ = static_cast<sr_t>(std::forward<T>(sr));
        return *this;
    }

    struct __brkn_pkg
    setErrPrtCB(callback_t && cb) noexcept {
        this->cont_ = std::move(cb);
        return *this;
    }

    llvm::StringRef operator()(void) noexcept {
        return std::visit([](auto && rhs) -> llvm::StringRef {
            using arg_t = typename std::remove_reference_t<decltype(rhs)>;

            if constexpr (std::is_same_v<arg_t, callback_t>)
                return static_cast<callback_t>(rhs)();

            if constexpr (std::is_same_v<arg_t, llvm::StringRef>)
                return static_cast<llvm::StringRef>(rhs);

            return __BRKN_PKG_NULL_STRING;
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

template <typename T>
struct _Package {
    std::variant<T, BrokenPackage> package_content_;

    template <typename _T>
    _Package(_T && val)
    : package_content_(std::forward<_T>(val)) {}

    template <typename _T>
    static auto
    makeBroken(_T&& err) noexcept {
        static_assert(
            std::disjunction_v<
                std::is_convertible<_T, llvm::StringRef>,
                std::is_convertible<_T, llvm::StringRef>
            >,  "Parameter _T&& err should be "
                "either llvm::StringRef constructible or convertible"
        );

        return _Package<T>(
            BrokenPackage{}.setErrPrtCB(std::forward<_T>(err)));
    }

    // Package can have Nothing, and since has the callback function,
    // the callback calling could be happened after opening.
    std::optional<T> open() noexcept {
        return \
        std::visit([](auto && arg) -> std::optional<T> {
            using arg_t = typename std::remove_reference_t<decltype(arg)>;

            if constexpr (std::is_same_v<arg_t, T>)
                return static_cast<T>(arg);

            if constexpr (std::is_same_v<arg_t, BrokenPackage>) {
                if(llvm::StringRef errMsg = static_cast<BrokenPackage>(arg)();
                errMsg == __BRKN_PKG_NULL_STRING)
                    spdlog::error("Error while opening package"
                    "; broken package returned error message for nothing.");
                else if(!errMsg.empty()) spdlog::warn(errMsg.str());

                return {};
            }

            spdlog::error(
            "Failed to open package; package has neither expected content nor error callback:"
            " replacing task by instantly default constructed.");
            return {};
        }, this->package_content_);
    }

    inline auto operator()(void) noexcept { return this->open(); }
};

// the handling callback function is required.
template <typename T>
using Package = struct _Package<T>;

NSRCXEND

#endif