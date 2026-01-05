#ifndef RCX_CONV_VARIANTICAL_HPP
#define RCX_CONV_VARIANTICAL_HPP

#include <variant>

#include <conv/Modernizer.hpp>

NSRCXBGN
namespace variant {
  
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

} // ns variant
NSRCXEND

#endif
