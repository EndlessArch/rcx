#ifndef RCX_CONV_MODERNIZER_HPP
#define RCX_CONV_MODERNIZER_HPP

//#ifndef RCX_INCL_ARGPARSE_H
//#define RCX_INCL_ARGPARSE_H
//#include <argparse.h>
//#endif // RCX_INCL_ARGPARSE_H

#include <condition_variable>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Optional.h>

#include <spdlog/spdlog.h>

#define NSRCXBGN \
namespace rcx {

#define NSRCXEND \
} // ns rcx

NSRCXBGN

// template <typename T, template<typename...> typename U>
// struct is_kind_of : std::false_type {};

// template <template<typename...> typename T,
//     template<typename...> typename U>
// struct is_kind_of<T, T> : std::true_type {};

// template <typename T, template<typename...> typename U>
// constexpr bool is_kind_of_v = is_kind_of<T, U>::value;

template <typename T /* FROM */, typename U /* TO */,
    typename R = std::remove_reference_t<T>>
struct is_basically : std::is_same<R, U> {};

template <typename T, typename U>
constexpr bool is_basically_v = is_basically<T, U>::value;

template <typename T, typename... Us>
T ctr_from(Us... args) noexcept {
    return T(args...);
}

template <typename T, typename... Args>
struct has : std::false_type {};

template <template<typename...> typename V, typename T, typename... Args>
struct has<T, V<T, Args...>> : std::true_type {};

template <template<typename...> typename V, typename T, typename U, typename... Args>
struct has<T, V<U, Args...>> : has<T, V<Args...>> {};

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

        // this->cont_ = static_cast<sr_t>(std::forward<T>(sr));
        this->cont_ = std::forward<sr_t>(sr);
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

} // ns anon

template <typename T>
struct _Package {
    using content_type = T;

    std::variant<T, BrokenPackage> package_content_;

    template <typename _T>
    _Package(_T && val)
    : package_content_(std::forward<_T>(val)) {}

    template <typename _T>
    static auto
    makeBroken(_T&& err) noexcept {
        static_assert(
            std::disjunction_v<
                rcx::is_basically<_T, llvm::StringRef>,
                std::is_constructible<llvm::StringRef, _T>,
                std::is_convertible<_T, llvm::StringRef>
            >,  "Parameter `err` should be "
                "either llvm::StringRef constructible or convertible"
        );

        return _Package<T>(
            BrokenPackage{}.setErrPrtCB(std::forward<_T>(err)));
    }

    T&& operator*() noexcept {
        return \
        std::forward<T>(
            std::visit([](auto&& arg) -> T&& {
                using arg_t = typename std::remove_reference_t<decltype(arg)>;

                if constexpr (std::is_same_v<arg_t, T>)
                    return std::forward<T>(arg);

                return std::move(T{});
            }, package_content_));
    }

    // Package can have Nothing, and since has the callback function,
    // the callback calling could be happened after opening.
    std::optional<T> open() noexcept {
        return \
        std::visit([](auto && arg) -> std::optional<T> {
            using arg_t = typename std::remove_reference_t<decltype(arg)>;

            if constexpr (std::is_same_v<arg_t, T>)
                return std::forward<T>(arg);

            if constexpr (std::is_same_v<arg_t, BrokenPackage>) {
                if(llvm::StringRef errMsg = std::forward<BrokenPackage>(arg)();
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

    constexpr bool hasValue() noexcept {
        return std::visit([](auto&& arg) {
            using arg_t = std::remove_reference_t<decltype(arg)>;

            if constexpr (std::is_same_v<arg_t, BrokenPackage>)
                return false;
            return true;
        }, package_content_);
    }

    template <typename _T, typename F,
        typename O = std::invoke_result_t<F, T>,
        typename = std::enable_if_t<
            std::disjunction_v<
                rcx::is_basically<O, _T>,
                std::is_constructible<_T, O>,
                std::is_convertible<O, _T> >> >
    static
    _Package<_T>&& transform(_Package<T>&& p, F&& f) noexcept {
        return std::visit([&f](auto&& arg) -> _Package<_T>&& {
            using arg_t = std::remove_reference_t<decltype(arg)>;

            if constexpr (std::is_same_v<arg_t, T>)
                return _Package<_T>{static_cast<_T>(f(std::forward<T>(arg)))};

            return _Package<_T>::makeBroken(std::forward<BrokenPackage>(arg));
        }, p.package_content_);
    }

    template <typename _U, typename _T = T>
    static
    _Package<_T>&& cast(_Package<_U>&& p) noexcept {
        return _Package<_U>::template transform<_T>(
            std::forward<_Package<_U>>(p),
            [](auto&& ctt) noexcept -> _T&& {
                return std::forward<_T>(ctt);
            } );
    }
};

// the handling callback function is required.
template <typename T>
using Package = struct _Package<T>;

NSRCXEND

#endif // RCX_CONV_MODERNIZER_HPP
