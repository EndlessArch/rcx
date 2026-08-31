#ifndef RCX_PARSE_PARSER_TPP
#define RCX_PARSE_PARSER_TPP

#include <iostream>

#include <conv/Callable.hpp>
#include <conv/Tuples.hpp>
#include <parse/parser.hpp>
#include <type_traits>

// fuck it

NSRCXBGN

namespace parser {
using namespace ctx;

using type_t = ast::Type;
using expr_t = parser::expr_t;
using arglist_t = std::vector<std::pair<type_t, std::string>>;

// NOTE: Once thought about layout concept;
// Which preceded arranging data/types; Layout -> Context -> AST
struct Layout {};

struct LAttrs : Layout {
    std::vector<std::string> vAttrs_;
    LAttrs() : Layout(), vAttrs_{} {}
};

struct LFunction : Layout {
    LAttrs fAttrs_;
    std::string fName_;
    metavars_t fMVars_;
    arglist_t fArgs_;
    std::vector<expr_t> fExprs_;

    LFunction(std::string fName, metavars_t fMVars,
        arglist_t fArgs, std::vector<expr_t> fExprs, LAttrs&& fAttrs = {}) : Layout(),
        fName_(fName), fMVars_(fMVars), fArgs_(fArgs),
        fExprs_(fExprs), fAttrs_{std::move(fAttrs)} {}
};

struct LClass : Layout {
    using members_t = std::vector<std::pair<std::string, expr_t>>;

    LAttrs cAttrs_;
    std::string cName_;
    metavars_t cMVars_;
    members_t cMembers_;
    std::vector<LFunction> cMethods_;

    LClass(std::string cName, metavars_t cMVars,
        members_t cMembers, std::vector<LFunction> cMethods, LAttrs&& cAttrs = {}) : Layout(),
        cName_{cName}, cMVars_{cMVars}, cMembers_{cMembers}, cMethods_{cMethods},
        cAttrs_{std::move(cAttrs)} {}
};

namespace {

// Token / string / pair
template <typename Read, typename Expect>
constexpr bool
matchesReadResult(const Read& got, const Expect& expected) noexcept {
    const auto& [tok, idf] = got;

    using tok_t = std::decay_t<decltype(tok)>;
    using idf_t = std::decay_t<decltype(idf)>; // be likely std::string

    using cdd_t = rcx::candidates<idf_t, char>;

    if constexpr(rcx::is_basically_v<Expect, Token>) {
        return tok == expected;
    }

    if constexpr(std::is_same_v<std::decay_t<Expect>, char>) {
        return idf.length() == 1 && idf[0] == static_cast<char>(expected);
    }

    // directly convertible
    if constexpr(std::is_convertible_v<Expect, std::string>) {
        return idf == expected;
    }

    if constexpr(tupl::is_pair_v<Expect>) {
        if constexpr(tupl::has_pair_v<tok_t, Expect>
                && tupl::has_pair_v<cdd_t, Expect>) {
            ;
            // const auto& [_tok, _idf] = expected;
            const auto& _tok = std::get<tok_t>(expected);
            
            using ii_t = tupl::get_other_t<tok_t, Expect>;
            const auto& _idf = std::get<ii_t>(expected);
            return tok == _tok && idf == _idf;
        }

        return matchesReadResult(got, expected.first)
            && matchesReadResult(got, expected.second);
    }

    if constexpr(rcx::Iterable<Expect>) {
        // return std::all_of(expected.begin(), expected.end(),
        //     [&got](const auto& item) noexcept {
        //         return matchesReadResult(got, item);
        //     });
        return std::any_of(expected.begin(), expected.end(),
            std::bind_front(
                /* typename Expect::value_type */
                matchesReadResult<Read, typename std::ranges::range_value_t<Expect>>,
                std::cref(got)));
    }
    
    return false;
}

} // ns anon

template <template<typename...> typename V, typename VE,
    typename F, typename U, typename M>
Package<V<std::remove_reference_t<VE>>>
parseDynamicArguments(F&& readF, U&& expectF, auto&& til, M&& modF) noexcept {
    using array_t = V<std::remove_reference_t<VE>>;
    using read_result_t = std::invoke_result_t<F&>;
    using expected_result_t = std::invoke_result_t<U&, std::string>;

    static_assert(std::is_default_constructible_v<array_t>,
        "`parseArray` result container must be default constructible");
    static_assert(std::is_invocable_v<M, array_t&, VE&&>,
        "`parseArray` append callback must accept the container and parsed element");

    array_t arr{};
    std::optional<read_result_t> buffered{};

    // return already buffered one if exists
    auto readBuffered = [&readF, &buffered]() noexcept -> read_result_t {
        if(buffered.has_value()) {
            auto a = std::move(*buffered);
            buffered.reset();
            return a;
        }

        return std::invoke(readF);
    };

    // Expected argument form: ( argumentName : argumentType , ... )
    for(unsigned cycle = 0; cycle < 512; ++cycle) {
        buffered = readBuffered();
        if(matchesReadResult(*buffered, til)) {
            buffered.reset();
            return Package<array_t>(std::move(arr));
        }

        // read name
        buffered = readBuffered();
        if(!matchesReadResult(*buffered, Token::Identifier)) {
            auto&& [tok, idf] = *buffered;
            return Package<array_t>::makeBroken(
                fmt::format("Expected identifier, found {} '{}'", stringifyTok(tok), idf));
        }
        auto id_argname = std::move(buffered->second);
        buffered.reset();

        // read :
        buffered = readBuffered();
        if(!matchesReadResult(*buffered, Token::Type)) {
            auto&& [tok, idf] = *buffered;
            return Package<array_t>::makeBroken(
                fmt::format("Expected ':', found {} '{}'", stringifyTok(tok), idf));
        } else buffered.reset();
        
        // read type
        buffered = readBuffered();
        // using bitmask ?
        // Token::Identifier | Token::...
        if(!matchesReadResult(*buffered, std::array{Token::Identifier, Token::TypeDyn})) {
            auto&& [tok, idf] = *buffered;
            return Package<array_t>::makeBroken(
                fmt::format("Expected identifier, found {} '{}'", stringifyTok(tok), idf));
        }
        auto id_argtype = std::move(buffered->second);
        buffered.reset();

        std::invoke(modF, arr, std::pair{id_argname, id_argtype});

        buffered = readBuffered();
        if(!matchesReadResult(*buffered, Token::Comma)) {
            if(!matchesReadResult(*buffered, til)) {
                const auto& [tok, idf] = *buffered;
                return Package<array_t>::makeBroken(
                    fmt::format("Expected comma or array terminator, found {} '{}'",
                        stringifyTok(tok), idf));
            }
            buffered.reset();
            return Package<array_t>{std::move(arr)};
        }
        buffered.reset();
    }

    return Package<array_t>::makeBroken("Too much array elements");
}

template <template<typename...> typename V, typename F, typename U,
    typename TE, typename TS, typename TT,
    typename = std::enable_if_t<
        std::conjunction_v<
            std::is_default_constructible<V<std::string>>,
            std::disjunction<
                std::is_invocable<U, V<std::string>&, const std::string&>,
                std::is_invocable<U, V<std::string>&, std::string&&>> >,
            std::conjunction<
                std::negation<std::is_function<std::decay_t<TE>>>,
                std::negation<std::is_function<std::decay_t<TS>>>,
                std::negation<std::is_function<std::decay_t<TT>>> >
            >>
Package<V<std::string>>
parseArray(F&& readF, TE&& elemType, TS&& seprType, TT&& termType, U&& appendF) noexcept {
    using array_t = V<std::string>;
    using read_result_t = std::invoke_result_t<F&>; // std::pair

    // static_assert(rcx::callable::CallableObj<TE>);
    // static_assert(std::is_invocable<std::decay_t<TS>>::value); // false
    // static_assert(std::is_invocable<std::decay_t<TT>>::value); // false

    array_t arr{};

    spdlog::debug("STILL");

    for(unsigned i = 0; i < 1024; ++i) {
        read_result_t got = std::invoke(readF);
        if(matchesReadResult(got, termType))
            return Package<array_t>(std::move(arr));

        if(!matchesReadResult(got, elemType)) {
            const auto& [tok, idf] = got;
            return Package<array_t>::makeBroken(
                fmt::format("Expected array element, not {} '{}'",
                    stringifyTok(tok), idf));
        }
        // NOTE: Token abandoned
        std::invoke(appendF, arr, std::move(got.second));

        got = std::invoke(readF);

        if(matchesReadResult(got, termType))
            return Package<array_t>(std::move(arr));

        if(!matchesReadResult(got, seprType)) {
            const auto& [tok, idf] = got;
            spdlog::debug("Expected array separator, found {} '{}'",
                    stringifyTok(tok), idf);
            return Package<array_t>::makeBroken(
                fmt::format("Expected array separator, found {} '{}'",
                    stringifyTok(tok), idf));
        }
    }

    return Package<array_t>::makeBroken("Too much array elements");
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

template <typename T, typename U>
Package<ExpressionContext>
parseType(T&& readF, U&& expectF) noexcept {
    // TODO: make it capable parsing `some::name::space::Type`
}

template <typename T, typename U, typename TT>
Package<ExpressionContext>
parseExpression(T&& readF, U&& expectF, ctx::SpaceContext& sc, TT&& termType) noexcept {
    auto is_ret = expectF(Token::KeyReturn);

    if(is_ret) {
        ExpressionContext ret{};
        return Package<ExpressionContext>{std::move(ret)};
    }

    auto&& [tok, idf] = *is_ret;

    if(Token::Identifier != tok) {
        return Package<ExpressionContext>::makeBroken("Expected expression");
    }

    auto is_tp_def = expectF(Token::Type);
    if(is_tp_def) {
        // define or define&init
        // string:(possibly::name_space::)type_name (= value_or_expression)
        auto type = parseType(std::forward<T>(readF), std::forward<U>(expectF));
        if(!type) {
            std::move(type).open();
            return Package<ExpressionContext>::makeBroken("Failed to parse variable type definition");
        }

        auto is_set = expectF(Token::VSet);
        if(!is_set) {
            ExpressionContext ec{idf, *std::move(type)()};
            return Package<ExpressionContext>{std::move(ec)};
        }

        auto value = parseExpression(std::forward<T>(readF), std::forward<U>(expectF), sc, std::forward<TT>(termType));
        ExpressionContext ec{idf, *std::move(type)(), *std::move(value)()};
        return Package<ExpressionContext>{std::move(ec)};
    }

    auto is_set = expectF(Token::VSet);
    if(is_set) {
        auto value = parseExpression(std::forward<T>(readF), std::forward<U>(expectF), sc, std::forward<TT>(termType));
        ExpressionContext ec = ExpressionContext::Set(idf, *std::move(value)());
        return Package<ExpressionContext>{std::move(ec)};
    }

    return Package<ExpressionContext>::makeBroken("Unexpected expression");
}

template <typename T, typename U, typename TT>
Package<ctx::SpaceContext>
parseExpressions(T&& readF, U&& expectF, ctx::SpaceContext&, TT&& termType) noexcept {
    ctx::SpaceContext spc_exprs;
    for(unsigned i = 0; i< 1024; ++i) {
        auto pkg_expr = parseExpression(std::forward<T>(readF), std::forward<U>(expectF), spc_exprs, std::forward<TT>(termType));
        
        spdlog::debug("0");
        if(pkg_expr) {
            spdlog::debug("1");
            std::move(pkg_expr).open();
            spdlog::debug("2");
            return Package<ctx::SpaceContext>::makeBroken("Failed to parse expression");
        }
        
        spdlog::debug("addDef 1");
        spc_exprs.addDef(*std::move(pkg_expr)());
        spdlog::debug("addDef 2");
    }

    return Package<ctx::SpaceContext>{std::move(spc_exprs)};
}

}

using namespace parser;
using annos_t = std::vector<std::string>;

template <typename T, typename U>
Package<ctx::FunctionContext>
parseFunction(T&& readF, U&& expectF, ctx::SpaceContext& ctx,
    std::optional<annos_t>&& annos = {}) noexcept {
    ;

    // TODO: annos

    {
        auto pkg_arg_open = expectF('(');
        if(!pkg_arg_open.hasValue()) {
            return Package<ctx::FunctionContext>::makeBroken("Expected function argument list");
        }
    }

    // auto pkg_arr = parseArray<std::vector, T>(
    //     std::forward<T>(readF), Token::Identifier, Token::Comma,
    //     std::pair{ Token::Braces, '}' },
    //     [](auto& v, auto&& a) { v.push_back(std::forward<decltype(a)>(a)); });

    auto pkg_args = parseDynamicArguments<std::vector, std::pair<std::string, std::string>>(
        std::forward<T>(readF), std::forward<U>(expectF),
        std::pair{ Token::Parentheses, ')' },
        [](auto& v, auto&& a) { v.push_back(std::forward<decltype(a)>(a)); });

    if(!pkg_args.hasValue()) {
        std::move(pkg_args).open();
        return Package<ctx::FunctionContext>::makeBroken("Failed to parse function arguments");
    }

    auto args = *pkg_args; // vector<pair<string, string>>

    {
        auto pkg_arg_type_guide = expectF(Token::TypeArrow);
        if(!pkg_arg_type_guide) {
            std::move(pkg_arg_type_guide).open();
            return Package<ctx::FunctionContext>::makeBroken("Failed to read function return type: expected '->'");
        }
    }

    auto pkg_arg_type_name = expectF(Token::Identifier);
    if(!pkg_arg_type_name) {
        std::move(pkg_arg_type_name).open();
        return Package<ctx::FunctionContext>::makeBroken("Failed to read function return type: expected return type");
    }
    auto arg_rtype = *pkg_arg_type_name;

    {
        auto pkg_body_open = expectF('{');
        if(!pkg_body_open) {
            std::move(pkg_body_open).open();
            return Package<ctx::FunctionContext>::makeBroken("Failed to read function: expected '{'");
        }
    }
    
    // TODO: specialized function for parsing function body is necessary
    ctx::SpaceContext ns_space("", {});
    // auto pkg_ns = parseGlobal(std::forward<T>(readF), std::forward<U>(expectF), ns_space);
    auto pkg_ex = parseExpressions(std::forward<T>(readF), std::forward<U>(expectF), ns_space, std::pair{ Token::Braces, '}' });
    if(!pkg_ex) {
        std::move(pkg_ex).open();
        return Package<ctx::FunctionContext>::makeBroken("Failed to read function body");
    }

    auto pkg_body_close = expectF('}');
    if(!pkg_body_close) {
        return Package<ctx::FunctionContext>::makeBroken("Failed to read function: expected '}'");
    }

    ctx::FunctionContext fc{"", std::move(args), std::move(ns_space), annos};

    return Package<ctx::FunctionContext>{std::move(fc)};
    // return Package<ctx::FunctionContext>::makeBroken("Function parser is not implemented");
}

template <typename T, typename U>
Package<ctx::ClassContext>
parseStruct(T&& readF, U&& expectF, ctx::SpaceContext&) noexcept {
    return Package<ctx::ClassContext>::makeBroken("Struct parser is not implemented");
}

template <typename T, typename U>
Package<ctx::FunctionContext>
parseBinaryFunction(T&& readF, U&& expectF, ctx::SpaceContext&) noexcept {
    return Package<ctx::FunctionContext>::makeBroken("Binary function parser is not implemented");
}

template <typename T, typename U>
Package<annos_t>
parseAnnotation(T&& readF, U&& expectF, ctx::SpaceContext& ctx) noexcept {
    ;
    auto pkg_open = expectF('{');
    if(!pkg_open) {
        return Package<annos_t>::makeBroken("Expected annotation list");
    }

    auto pkg_arr = parseArray<std::vector, T>(
        std::forward<T>(readF), Token::Identifier, Token::Comma,
        std::pair{ Token::Braces, '}' },
        [](auto& v, auto&& a) { v.push_back(std::forward<decltype(a)>(a)); });

    if(pkg_arr) {
        auto arr = std::move(pkg_arr).open();
        spdlog::debug("Parsed {} attributes", arr->size());
    } else {
        spdlog::warn("Failed to parse array");
        std::move(pkg_arr).open(); // 1: parsearray
    }

    return Package<annos_t>::cast<annos_t>(std::move(pkg_arr));
}

template <typename T, typename U>
Package<ctx::SpaceContext>
parseNamespace(T&& readF, U&& expectF, ctx::SpaceContext& ctxFrom) noexcept {

    auto pkg_name = expectF(Token::Identifier);
    if(!pkg_name) {
        // return Package<ctx::SpaceContext>::makeBroken("Expected namespace name");
        return Package<ctx::SpaceContext>(*pkg_name.extract());
    }

    auto name = std::move(pkg_name).open();
    if(!name) {
        return Package<ctx::SpaceContext>::makeBroken("Failed to open namespace name");
    }
    spdlog::debug("namespace name = {}", (*name).second);

    auto pkg_open = expectF('{');
    if(!pkg_open) {
        std::move(pkg_open).open();
        return Package<ctx::SpaceContext>::makeBroken("Expected namespace body");
    }

    ctx::SpaceContext ns_space(name->second, {});
    auto pkg_ns = parseGlobal(std::forward<T>(readF), std::forward<U>(expectF), ns_space);
    if(!pkg_ns) {
        return pkg_ns;
    }

    auto pkg_close = expectF('}');
    if(!pkg_close) {
        spdlog::debug("Expected closing bracket from namespace '{}'", (*std::move(pkg_name)()).second);
        return Package<ctx::SpaceContext>::makeBroken("Expected namespace close");
    }

    ctxFrom.addDef(std::move(*pkg_ns));
    return Package<ctx::SpaceContext>(std::move(ns_space));
}

template <typename T, typename U>
Package<ctx::SpaceContext>
parseGlobal(T&& readF, U&& expectF, ctx::SpaceContext& ctxFrom) noexcept {
    ;
    // using type_t = ast::Type;
    // using expr_t = parser::expr_t;

    using metalist_t = parser::metavars_t;
    // using arglist_t = std::vector<std::pair<type_t, std::string>>;

    // annos, name, meta variables, arguments, expressions
    // using fn_t = std::tuple<
    //     annos_t, std::string,
    //     metalist_t, arglist_t,
    //     std::vector<expr_t> >;
    using fn_t = LFunction;
    // annos, name, meta variable, members&init values, methods
    // using cls_t = std::tuple<
    //     annos_t, std::string,
    //     metalist_t,
    //     std::vector<std::pair<std::string, expr_t>>,
    //     std::vector<fn_t> >;
    using cls_t = LClass;

    using property_t = std::variant<annos_t, fn_t, cls_t>;

    std::optional<property_t> parse_buf{};

    do {
        spdlog::debug("CONTEXT DUMP -- {");
        ctxFrom.dumpDef();
        spdlog::debug("CONTEXT DUMP -- }");

        auto [ tok, idf ] = readF();
        std::cout << idf << ": <" << stringifyTok(tok) << ">\n";
        if(Token::__EOF == tok || Token::__UNKNOWN == tok) break;

        // there's no known parsed metadata/property to continue.
        if (!parse_buf.has_value()) {
            // function parsing starts with the name
            if(tok == Token::Identifier) {
                spdlog::debug("Try parsing function {} with no attributes", idf);
                // parse_buf = fn_t({}, idf, {}, {}, {});
                auto pkg_fn = parseFunction(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                auto fn = std::move(pkg_fn)();
                if(fn.has_value()) ctxFrom.addDef((std::move(*fn)));
                else spdlog::error("parseGlobal: Failed to open package, Function");

                std::cin.get();
                continue;
            }

            if(tok == Token::KeyStruct) {
                spdlog::debug("Try parsing struct {} with no attributes", idf);
                // parse_buf = cls_t({}, "", {}, {}, {});
                auto pkg_st = parseStruct(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                auto st = std::move(pkg_st)();
                if(st.has_value()) ctxFrom.addDef(std::make_shared<ctx::ClassContext>(std::move(*st)));
                else spdlog::error("parseGlobal: Failed to open package, Struct");
		        continue;
            }

            // may be case of binary function definition
            if(idf == "(") {
                spdlog::debug("Try parsing binary function {} with no attributes", idf);
                // parse_buf = fn_t({}, fmt::format("({})", parse_expect(idf, Token::Identifier)), {}, {}, {});
                auto pkg_bfn = parseBinaryFunction(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                auto bfn = std::move(pkg_bfn)();
                if(bfn.has_value()) ctxFrom.addDef(std::make_shared<ctx::FunctionContext>(std::move(*bfn)));
                else spdlog::error("parseGlobal: Failed to open package, BinaryFunction");
                continue;
            }

            if(tok == Token::Annotation) {
                spdlog::debug("Try parsing attributes", idf);
                auto pkg_ann = parseAnnotation(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                spdlog::debug("---");
                if(pkg_ann)
                    parse_buf = (property_t)static_cast<annos_t>(*pkg_ann);
                // else spdlog::error("parseGlobal: Failed to open package, annotation");
                else std::move(pkg_ann).open();
                // spdlog::debug("PARSE ANNO END");
                continue;
            }

            if(tok == Token::Namespace) {
                spdlog::debug("Parsing namespace {}", idf);
                auto pkg_ns = parseNamespace(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                auto ns_ctx = std::move(pkg_ns).open();
                if(ns_ctx.has_value()) ctxFrom.addDef(std::make_shared<ctx::SpaceContext>(std::move(*ns_ctx)));
                else spdlog::error("parseGlobal: Failed to open package, Namespace");
                continue;
            }

            spdlog::error("Unexpected {}, \'{}\'", stringifyTok(tok), idf);
            continue;
        }
        else {
            // function with annotation
            if(tok == Token::Identifier) {
                spdlog::debug("Try parsing function {} with provided attributes", idf);

                bool is_anno = std::visit([](auto&& a) -> bool {
                    return std::is_same_v<std::decay_t<decltype(a)>, annos_t>;
                }, parse_buf.value());

                if (is_anno) {
                    auto&& a = std::get<annos_t>(parse_buf.value());
                    
                    auto pkg_fn = parseFunction(
                        std::forward<T>(readF),
                        std::forward<U>(expectF), ctxFrom, std::move(a));
                    auto fn = std::move(pkg_fn)();
                    if(fn.has_value()) ctxFrom.addDef((std::move(*fn)));
                    else spdlog::error("parseGlobal: Failed to open package, Function");

                    std::cin.get();
                    continue;
                }
                else {
                    ;
                }
            }
        }
    } while(1);

    return Package<ctx::SpaceContext>(std::move(ctxFrom));
}

NSRCXEND

#endif // RCX_PARSE_PARSER_TPP
