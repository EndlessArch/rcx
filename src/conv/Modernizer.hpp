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

#include <conv/Compare.hpp>

#include <boost/outcome.hpp>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Optional.h>

#include <spdlog/spdlog.h>

#define NSRCXBGN \
namespace rcx {

#define NSRCXEND \
} // ns rcx

NSRCXBGN

template <typename... Ts>
struct candidates;

template <template<typename...> typename T, typename U>
struct is_kind_of : std::false_type {};

template <template<typename...> typename T,
    typename U>
struct is_kind_of<T, T<U>> : std::true_type {};

template <template<typename...> typename T, typename U>
constexpr bool is_kind_of_v = is_kind_of<T, std::remove_cvref_t<U>>::value;

static_assert(is_kind_of_v<std::vector, std::vector<int>>);
static_assert(is_kind_of_v<std::shared_ptr, std::shared_ptr<std::shared_ptr<std::string>>>);

template <template<typename...> typename T, template<typename...> typename U>
struct is_each_kind_of : std::false_type {};

template <template<typename...> typename T>
struct is_each_kind_of<T, T> : std::true_type {};

template <template<typename...> typename T, template<typename...> typename U>
constexpr bool is_each_kind_of_v = is_each_kind_of<T, U>::value || is_each_kind_of<U, T>::value;

template <typename T /* FROM */, typename U /* TO */,
    typename R = std::remove_cvref_t<T> >
struct is_basically : std::is_same<R, U> {};

template <typename T, typename U>
constexpr bool is_basically_v = is_basically<T, U>::value;

// template <class F1, class F2>
// struct overloaded : F1, F2 {
//     overloaded(F1 x, F2 y) : F1(x), F2(y) {}

//     using F1::operator();
//     using F2::operator();
// };

// template <class F1, class F2>
// inline
// overloaded<F1, F2> overload(F1 f1, F2 f2) noexcept {
//     return overloaded<F1, F2>(f1, f2);
// }

// NOTE: the project is using c++20
template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// . <17
// template<class... Ts>
// overloaded(Ts...) -> overloaded<Ts...>;

template <typename T, typename... Us>
T ctr_from(Us... args) noexcept {
    return T(args...);
}

namespace {

template <typename T>
struct extract_return_t {};

template <typename R, typename... Args>
struct extract_return_t<R(Args...)> {
    using type = R;
};

} // ns anon, function_return_t

// allows return type inference (without providing arguments type)
template <auto F>
using invoke_return_t = typename extract_return_t<decltype(std::function{F})>::type;

// template <typename L>
// using invokeT_return_t = extract_return_t<decltype(L)>::type; // invoke_return_t<std::declval<L>()>;

template <typename T, typename... Args>
struct has : std::false_type {};

template <template<typename...> typename V, typename T, typename... Args>
struct has<T, V<T, Args...>> : std::true_type {};

template <template<typename...> typename V, typename T, typename U, typename... Args>
struct has<T, V<U, Args...>> : has<T, V<Args...>> {};

namespace {

// #define __BRKN_PKG_NULL_STRING "\xd"

using BrokenPackage = struct __brkn_pkg {
    using callback_t = typename std::function<std::string(void)>;

    // either (error) message or message constructor
    std::variant<std::string, callback_t> cont_;

    template <typename T, typename = std::enable_if_t<std::is_convertible_v<T, std::string>>>
    struct __brkn_pkg
    setErrPrtCB(T && sr) noexcept {
        // NOTE: StringRef doesn't own characters
        this->cont_ = std::string{std::forward<T>(sr)};
        return *this;
    }

    struct __brkn_pkg
    setErrPrtCB(callback_t && cb) noexcept {
        this->cont_ = std::forward<callback_t>(cb);
        return *this;
    }

    std::string_view operator()(void) noexcept {
        return std::visit([](auto && rhs) -> std::string_view {
            using arg_t = typename std::remove_cvref_t<decltype(rhs)>;

            if constexpr (std::is_same_v<arg_t, callback_t>) {
                // spdlog::debug("1");
                return std::string_view{std::forward<std::string>(std::invoke(rhs))};
            }

            if constexpr (std::is_same_v<arg_t, std::string>) {
                spdlog::debug("rhs: {}", rhs.data());
                return static_cast<std::string_view>(rhs);
            }

            return std::string_view{};
        }, cont_);
    }
};

} // ns anon

template <typename T>
struct _Package {
    using content_type = T;

    std::variant<T, BrokenPackage> package_content_;

    _Package(const _Package<T>&) = default;
    _Package(_Package<T>&&) = default;

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
            BrokenPackage{}.setErrPrtCB<_T>(std::forward<_T>(err)));
    }

    // Package can have Nothing, and since has the callback function,
    // the callback calling could be happened after opening.
    std::optional<T> open() noexcept {
        return \
        std::visit(/*[](auto && arg) -> std::optional<T> {
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
            return {};*/
        overloaded {
            [](const T& arg) -> std::optional<T> { return arg; },
            [](const BrokenPackage& brk) -> std::optional<T> { return {}; }
        }, package_content_);
    }

    inline
    std::optional<T> operator()(void) noexcept {
        return std::visit(
            overloaded {
                [](T&& arg) noexcept -> std::optional<T> { return std::move(arg); },
                [](BrokenPackage&& bp) noexcept -> std::optional<T> {
                    if(llvm::StringRef errMsg = bp(); errMsg.empty())
                        spdlog::error("Error while opening package"
                        "; broken package returned error message for nothing.");
                    // else if(!errMsg.empty()) spdlog::warn(errMsg.str());
                    return {};
                }
            }
        , std::move(package_content_));
    }

    inline constexpr
    operator bool(void) const noexcept { return this->hasValue(); }

    constexpr bool hasValue() const noexcept {
        return std::visit(
            overloaded{
                [](const BrokenPackage&) -> bool { return false; },
                [](auto&&) -> bool { return true; }
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
    _Package<_T> transform(_Package<T>&& p, F&& f) noexcept {
        return std::visit/*<_Package<_T>>*/(/*[](auto&&f, auto&& arg) -> _Package<_T> {
            using arg_t = std::remove_reference_t<decltype(arg)>;

            if constexpr (std::is_same_v<arg_t, T>)
                return _Package<_T>{f(std::forward<T>(arg))};
            
            // what's the point of casting atp?
            if constexpr (std::is_same_v<arg_t, BrokenPackage>)
                return _Package<_T>(std::forward<BrokenPackage>(arg));
                // return _Package<_T>::makeBroken(std::forward<BrokenPackage>(arg));

            // static constexpr llvm::StringRef errMsg = "Package::transform: Unexpected argument type";
            // return _Package<_T>::template makeBroken<const llvm::StringRef>(errMsg);
            return _Package<_T>::makeBroken("Package::transform: Unexpected argument type");*/
        overloaded {
            [&f](T&& t) { return _Package<_T>{f(std::forward<T>(t))}; } ,
            [&f](BrokenPackage&& brk) { return _Package<_T>(brk); }
        }, std::move(p.package_content_));
    }

    template <typename _U, typename _T = T>
    static
    _Package<_T> cast(_Package<_U>&& p) noexcept {
        // bool b = std::visit([](auto&& a) {
        //     if constexpr (rcx::is_basically_v<decltype(a), BrokenPackage>) {
        //         return true;
        //     }
        //     return false;
        // }, p.package_content_);

        // if (b)
        //     return _Package<_T>(static_cast<BrokenPackage&>(p.package_content_));

        // return trycast<_U, _T>(std::forward<_U>(p.package_content_));

        return std::visit(overloaded {
            [](_U&& p) {
                return _Package<_U>::template transform<_T>(
                    std::forward<_Package<_U>>(p),
                    [](auto&& ctt) noexcept -> _T {
                        return static_cast<_T>(std::forward<_U>(ctt));
                    });
            },
            [](BrokenPackage&& b) {
                return _Package<_T>(std::move(b));
            }
        }, std::move(p.package_content_));
    }

    std::optional<BrokenPackage>
    extract() noexcept {
        if(std::holds_alternative<BrokenPackage>(package_content_))
            return std::get<BrokenPackage>(std::move(package_content_));
        return {};
        // return std::visit(overloaded {
        //     [](T&&) noexcept -> std::optional<BrokenPackage> {
        //         return {};
        //     },
        //     [](BrokenPackage&& b) noexcept -> std::optional<BrokenPackage> {
        //         return std::move(b);
        //     }
        // }, std::move(package_content_));
    }
};

// the handling callback function is required.
// template <typename T>
// using Package = struct _Package<T>;

// distinguishes error type on Package<std::string>
struct PackageError : std::string {
    using std::string::string;

    PackageError(const std::string& error) : std::string(error) {}
    PackageError(std::string&& error) : std::string(std::move(error)) {}
};

template <typename T, typename U = PackageError>
// using Package = boost::outcome_v2::result<T, U>;
class Package {
public:
    using success_type = std::remove_cv_t<T>;
    using failure_type = std::remove_cv_t<U>;
    using wrapped_failure_type = boost::outcome_v2::failure_type<failure_type>;

protected:
    boost::outcome_v2::result<success_type, failure_type> result_;

private:
    template <typename _T>
    static failure_type to_error(_T&& error) noexcept {
        if constexpr (std::is_constructible_v<failure_type, _T>) {
            return failure_type{std::forward<_T>(error)};
        } else {
            static_assert(std::is_constructible_v<failure_type, std::string_view>,
                "package errors must be constructible from the propagated error type");
            return failure_type{"A packaged operation failed"};
        }
    }

    template <typename _T>
    static wrapped_failure_type cast_as_failure(_T&& error) noexcept {
        return boost::outcome_v2::failure(to_error(std::forward<_T>(error)));
    }

public:
    using result_type = decltype(result_);

    Package() noexcept
    : result_{cast_as_failure("empty package")} {}

    // Package(const boost::outcome_v2::result<T, U>& result) noexcept
    // : result_{result} {}

    template <typename _T>
    Package(_T&& value) noexcept
    : result_{[&]() -> result_type {
        if constexpr (std::is_same_v<std::remove_cvref_t<_T>, success_type>) {
            return result_type{boost::outcome_v2::success(std::forward<_T>(value))};
        } else {
            return result_type{cast_as_failure(std::forward<_T>(value))};
        }
    }()} {}

    template <typename _U>
    Package(boost::outcome_v2::failure_type<_U> failure) noexcept
    : result_{cast_as_failure(std::move(failure).error())} {}

    // template <typename _P, typename = std::enable_if_t<
    //     // T but not U
    //     (std::is_convertible_v<_P, T> && !std::is_convertible_v<_P, U>) ||
    //     // U but not T
    //     (std::is_convertible_v<_P, U> && !std::is_convertible_v<_P, T>)>>
    // Package(const _P& result) noexcept
    // : result_{ static_cast<
    //     std::conditional_t<
    //     (std::is_convertible_v<_P, T> && !std::is_convertible_v<_P, U>)
    //     , success_type
    //     , failure_type>>(result) } {}

    template <typename _T>
    static auto
    makeBroken(_T&& err) noexcept {
        static_assert(
            std::disjunction_v
                < rcx::is_basically<_T, U>
                , std::is_constructible<U, _T>
                , std::is_convertible<_T, U> >
                , "Parameter `err` should be "
                "either constructible or convertible to `U`"
        );

        return cast_as_failure(std::forward<_T>(err));
    }

    template <typename _T, typename _U>
    static auto
    makeBroken(_T&& err, _U&& f) noexcept {
        if constexpr (std::is_constructible_v<std::string_view, _U>) {
            return cast_as_failure(fmt::format("{}:\n{}", err, std::string_view{f}));
        }
        return cast_as_failure(fmt::format("{}", err));
    }

     template <typename _T, typename F,
        typename O = std::invoke_result_t<F, T>,
        typename = std::enable_if_t<
            std::disjunction_v<
                rcx::is_basically<O, _T>,
                std::is_constructible<_T, O>,
                std::is_convertible<O, _T> >> >
    static
    Package<_T> transform(Package<T>&& p, F&& f) noexcept {
        return p.result_
            ? Package<_T>{f(std::move(p.result_).assume_value())}
            : Package<_T>{boost::outcome_v2::failure(std::move(p.result_).assume_error())}
            ;

        // overloaded {
        //     [&f](T&& t) { return _Package<_T>{f(std::forward<T>(t))}; } ,
        //     [&f](BrokenPackage&& brk) { return _Package<_T>(brk); }
        // }, std::move(p.package_content_));
    }

    template <typename _U, typename _T = T>
    static
    Package<_T> cast(Package<_U>&& p) noexcept {
        return p.result_
            ? Package<_U>::template transform<_T>(
                std::move(p),
                [](auto&& a) noexcept -> _T {
                    return static_cast<_T>(std::forward<decltype(a)>(a));
                })
            : std::move(p)
            ;

        // return std::visit(overloaded {
        //     [](_U&& p) {
        //         return _Package<_U>::template transform<_T>(
        //             std::forward<_Package<_U>>(p),
        //             [](auto&& ctt) noexcept -> _T {
        //                 return static_cast<_T>(std::forward<_U>(ctt));
        //             });
        //     },
        //     [](BrokenPackage&& b) {
        //         return _Package<_T>(std::move(b));
        //     }
        // }, std::move(p.package_content_));
    }

    inline operator bool() const noexcept {
        return result_.has_value();
    }

    inline
    bool hasValue() const noexcept {
        return result_.has_value();
    }

    std::optional<wrapped_failure_type> extract() & noexcept {
        if (result_) return {};
        return boost::outcome_v2::failure(result_.assume_error());
    }

    std::optional<wrapped_failure_type> extract() && noexcept {
        if (result_) return {};
        return boost::outcome_v2::failure(std::move(result_).assume_error());
    }

    friend bool operator==(const Package& lhs, const Package& rhs) noexcept {
        if (lhs.result_ || rhs.result_) return false;
        if constexpr (requires (const failure_type& error) { error == error; }) {
            return lhs.result_.assume_error() == rhs.result_.assume_error();
        }
        return false;
    }

    inline
    auto getResult() noexcept {
        return result_;
    }
};

NSRCXEND

#endif // RCX_CONV_MODERNIZER_HPP
