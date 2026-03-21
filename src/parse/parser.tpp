#ifndef RCX_PARSE_PARSER_TPP
#define RCX_PARSE_PARSER_TPP

#include <iostream>

#include <conv/Tuples.hpp>
#include <parse/parser.hpp>

NSRCXBGN

namespace parser {

template <template<typename...> typename V, typename F, typename U,
    typename = std::enable_if_t<
        std::conjunction_v<
            std::is_default_constructible<V<std::string>>,
            std::disjunction<
                std::is_invocable<U, V<std::string>&, const std::string&>,
                std::is_invocable<U, V<std::string>&, std::string&&>>
            > >>
Package<V<std::string>>
parseArray(F&& readF, auto&& trg, auto&& spl, auto&& til, U&& appendF) noexcept {
    using array_t = V<std::string>;
    using trg_t = std::remove_reference_t<decltype(trg)>;
    using spl_t = std::remove_reference_t<decltype(spl)>;
    using til_t = std::remove_reference_t<decltype(til)>;

    static auto __lookif_end = [](auto&& l, auto&& _t, auto&& _i) noexcept
    -> bool {
        using rcx::tupl::has_tupl;
        using l_t = std::remove_reference_t<decltype(l)>;
        using i_t = std::remove_reference_t<decltype(_i)>;

        if constexpr(std::is_same_v<Token, l_t>) {
            Token l_tok = static_cast<Token>(std::forward<l_t>(l));
            if(l_tok == _t) return true;
        } else {
            if constexpr(tupl::is_tupl<l_t>) {
                if constexpr(auto n = has_tupl<Token>(std::declval<l_t>()) != -1) {
                    Token l_tok = static_cast<Token>(std::get<n>(l));
                    if(l_tok == _t) return true;
                }
                else if constexpr(auto n = has_tupl<i_t>(std::declval<i_t>()) != -1) {
                    auto l_i_t = static_cast<i_t>(std::get<n>(l));
                    if(l_i_t == _i) return true;
                }
                else {
                    spdlog::error("`l` is tuple but no viable Token found");
                    return false;
                }
            } else if constexpr(tupl::is_pair_v<l_t>) {
                if constexpr(tupl::is_pair_first_v<Token, l_t>) {
                    Token l_tok = static_cast<Token>(l.first);
                    if(l_tok == _t) return true;
                }
                else if constexpr(tupl::is_pair_second_v<Token, l_t>) {
                    Token l_tok = static_cast<Token>(l.second);
                    if(l_tok == _t) return true;
                }
                else {
                    spdlog::error("`l` is pair type but no Token found");
                    return false;
                }
            } else {
                spdlog::error("Unexpected type of `l`");
                return false;
            }
        }
    };
    
    auto [tok, idf] = readF();

    static auto fparse_arr =
    [tok, lookif_end = __lookif_end](const auto& idf, auto&& t, auto&& s, auto&& l, auto&& appendF) noexcept
    -> Package<array_t> {
        using t_t = std::remove_reference_t<decltype(t)>;
        using s_t = std::remove_reference_t<decltype(s)>;
        using l_t = std::remove_reference_t<decltype(l)>;

        V<std::string> v{};
        int cycle = 0;

        do {
            bool r0 = lookif_end(std::forward<l_t>(l), tok, idf);
            if(r0) return Package<array_t>(std::move(v));

            if(cycle%2 == 1) {
                if constexpr(std::is_same_v<Token, s_t>) {
                    Token s_tok = static_cast<Token>(std::forward<s_t>(s));
                    if(s_tok == tok) {
                        ++cycle;
                        continue;
                    }

                    bool r1 = lookif_end(std::forward<l_t>(l), tok, idf);
                    if(r1) return Package<array_t>(std::move(v));
                    return Package<array_t>::makeBroken(
                        fmt::format("Expected {} but {}", s_tok, tok));
                }
            } else {
                if constexpr(std::is_same_v<Token, t_t>) {
                    Token t_tok = static_cast<Token>(std::forward<t_t>(t));
                    if(t_tok == tok) {
                        appendF(v, (const std::string&)idf);
                        ++cycle;
                        continue;
                    }
                }
                else
                    return Package<array_t>::makeBroken("Unexpected type of `t`");
            }
        } while(cycle< 1024);

        return Package<array_t>::makeBroken("Too much array elements");
    };

    auto ret =
        std::visit(fparse_arr, idf,
            std::forward<trg_t>(trg),
            std::forward<spl_t>(spl),
            std::forward<til_t>(til),
            std::forward<U>(appendF));

    return static_cast<Package<array_t>>(ret);
}

// template <typename FL, typename FE, template<typename> class V,
//     typename = std::enable_if_t<
//         std::is_constructible_v<
//             V<std::string>, std::initializer_list<std::string> >> >
// Package<V<std::string>>
// parseVString(FL& lookupF, FE& expectF) noexcept {
//     auto [tok, idf] = lookupF();

//     expectF(tok, Token::Identifier);

//     parseArray
//     // lookupF(Token::Identifier, Token::Comma, { Token::Braces, '}' });
// }

}

using namespace parser;
using annos_t = std::vector<std::string>;

template <typename T, typename U>
Package<ctx::context_t>
parseFunction(T&& readF, U&& expectF, ctx::SpaceContext&) noexcept {
    ;
}

template <typename T, typename U>
Package<ctx::context_t>
parseStruct(T&& readF, U&& expectF, ctx::SpaceContext&) noexcept {

}

template <typename T, typename U>
Package<ctx::context_t>
parseBinaryFunction(T&& readF, U&& expectF, ctx::SpaceContext&) noexcept {

}

template <typename T, typename U>
Package<annos_t>
parseAnnotation(T&& readF, U&& expectF, ctx::SpaceContext&) noexcept {
    expectF('{');

    // parser::parseVString();
    auto pkg_arr
        = parseArray<std::vector, T>(
            std::forward<T>(readF), Token::Identifier, Token::Comma,
            std::pair{ Token::Braces, '}' },
            [](auto& v, auto&& a){ v.push_back(a); });
    
    if(pkg_arr.hasValue()) {
        ;
    }

    return Package<annos_t>::cast<annos_t>(std::move(pkg_arr));
}

template <typename T, typename U>
Package<ctx::context_t>
parseNamespace(T&& readF, U&& expectF, ctx::SpaceContext& ctxFrom) noexcept {
    std::string str;

    auto pkg_name = expectF(Token::Identifier);
    if(!pkg_name.hasValue()) {
        ;
    }
    auto pkg_open_name = pkg_name.open();
    auto name = pkg_open_name->second;

    ctx::SpaceContext ns_space(ctxFrom, name, {});

    expectF('{');

    parseGlobal(std::forward<T>(readF), std::forward<U>(expectF), ns_space);

    expectF('}');

    return Package<ctx::context_t>(std::move(ctxFrom));
}

template <typename T, typename U>
Package<ctx::context_t>
parseGlobal(T&& readF, U&& expectF, ctx::SpaceContext& ctxFrom) noexcept {
    ;
    using type_t = ast::Type;
    using expr_t = parser::expr_t;

    using metalist_t = parser::metavars_t;
    using arglist_t = std::vector<std::pair<type_t, std::string>>;
    // annos, name, meta variables, arguments, expressions
    using fn_t = std::tuple<
        annos_t, std::string,
        metalist_t, arglist_t,
        std::vector<expr_t> >;
    // annos, name, meta variable, members&init values, methods
    using cls_t = std::tuple<
        annos_t, std::string,
        metalist_t,
        std::vector<std::pair<std::string, expr_t>>,
        std::vector<fn_t> >;

    using property_t = std::variant<annos_t, fn_t, cls_t>;

    std::optional<property_t> parse_buf{};

    do {
        auto [ tok, idf ] = readF();
        std::cout << stringifyTok(tok) << '\n';
        if(Token::__EOF == tok) break;

        // there's no known parsed metadata/property to continue.
        if (!parse_buf.has_value()) {
            // function parsing starts with the name
            if(tok == Token::Identifier) {
                // parse_buf = fn_t({}, idf, {}, {}, {});
                auto pkg_fn = parseFunction(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                auto fn = pkg_fn();
                if(fn.has_value()) ctxFrom.addDef(*fn);
                else spdlog::error("parseGlobal: Failed to open package, Function");
                continue;
            }

            if(tok == Token::KeyStruct) {
                // parse_buf = cls_t({}, "", {}, {}, {});
                auto pkg_st = parseStruct(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                auto st = pkg_st();
                if(st.has_value()) ctxFrom.addDef(*st);
                else spdlog::error("parseGlobal: Failed to open package, Struct");
		        continue;
            }

            // may be case of binary function definition
            if(idf == "(") {
                // parse_buf = fn_t({}, fmt::format("({})", parse_expect(idf, Token::Identifier)), {}, {}, {});
                auto pkg_bfn = parseBinaryFunction(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                auto bfn = pkg_bfn();
                if(bfn.has_value()) ctxFrom.addDef(*bfn);
                else spdlog::error("parseGlobal: Failed to open package, BinaryFunction");
                continue;
            }

            if(tok == Token::Annotation) {
                auto pkg_ann = parseAnnotation(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                auto ann = pkg_ann();
                if(ann.has_value())
                    parse_buf = (property_t)static_cast<annos_t>(*ann);
                else spdlog::error("parseGlobal: Failed to open package, annotation");
                continue;
            }

            if(tok == Token::Namespace) {
                auto pkg_ns = parseNamespace(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                auto ns_ctx = pkg_ns();
                if(ns_ctx.has_value()) ctxFrom.addDef(*ns_ctx);
                else spdlog::error("parseGlobal: Failed to open package, Namespace");
                continue;
            }

            spdlog::error("Unexpected {}, \'{}\'", stringifyTok(tok), idf);
            continue;
        }
        else {
            ;
        }
    } while(1);

    return Package<ctx::context_t>(std::move(ctxFrom));
}

NSRCXEND

#endif // RCX_PARSE_PARSER_TPP