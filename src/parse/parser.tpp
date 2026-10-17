#ifndef RCX_PARSE_PARSER_TPP
#define RCX_PARSE_PARSER_TPP

#include <functional>
#include <iostream>
#include <type_traits>

#include <conv/Callable.hpp>
#include <conv/Tuples.hpp>
#include <parse/parser.hpp>
#include <parse/Layout/Layout.hpp>

// fuck it

NSRCXBGN

namespace parser {
using namespace ctx;

namespace {

// Token / string / pair
template <typename Read, typename Expect>
constexpr bool
matchesReadResult(const Read& got, const Expect& expected) noexcept {

    if constexpr (rcx::is_basically_v<Read, Token>
        && rcx::is_basically_v<Expect, EToken>) {
spdlog::debug("Token:EToken / {} ({}) == {}"
    , stringifyTok(static_cast<const Token&>(got).kind)
    , static_cast<const Token&>(got).lexeme
    , stringifyTok(static_cast<EToken>(expected)));
        return static_cast<const Token&>(got).kind == static_cast<EToken>(expected);
    }

    const auto& [tok, idf] = got;

    using tok_t = std::decay_t<decltype(tok)>;
    using idf_t = std::decay_t<decltype(idf)>; // be likely std::string

    using cdd_t = rcx::candidates<idf_t, char>;

    if constexpr(rcx::is_basically_v<Expect, EToken>) {
spdlog::debug("{} ; `{}`: {}", stringifyTok(expected), idf, stringifyTok(tok));
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
spdlog::debug("Tok: {} & {} ; Idf: {} & {}", stringifyTok(tok), stringifyTok(_tok), idf, _idf);
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
    // using expected_result_t = typename std::invoke_result_t<U&, EToken>::result_type;

    static_assert(std::is_default_constructible_v<array_t>,
        "`parseArray` result container must be default constructible");
    static_assert(std::is_invocable_v<M, array_t&, VE&&>,
        "`parseArray` append callback must accept the container and parsed element");

    array_t arr{};
    // std::optional<expected_result_t> buffered{};

    // Expected argument form: ( argumentName : argumentType , ... )
    // Using for-loop to prevent any potential infinite loop
    for(unsigned cycle = 0; cycle < 512; ++cycle) {
        auto r_term = expectF(til).getResult();
        if(r_term)
            return std::move(arr);

        // read name
        auto r_name = expectF(EToken::Identifier).getResult();
        if(!r_name) {
            // auto&& [t, i] = *buffered;
            // return Package<array_t>::makeBroken(
            //     fmt::format("Expected argument, found {} '{}'", stringifyTok(t), i));
            auto tok_errc = r_name.assume_error();
            return tok_errc.error;
        }
        auto argName = r_name.assume_value().lexeme;

        // read :
        auto r_ctype = expectF(':').getResult();
        if(!r_ctype)
            return Package<array_t>::makeBroken("Expected ':'", r_ctype.assume_error().error);
        
        // read type
        auto argType = parseNames(std::forward<F>(readF), std::forward<U>(expectF));

        std::string id_argtype;
        // using bitmask ?
        // Token::Identifier | Token::...
        if(!argType) {
            auto isDyn = expectF(EToken::TypeDyn).getResult();

            if(!isDyn) {
                // auto&& [t, i] = *buffered;
                // return Package<array_t>::makeBroken(
                //     fmt::format("Expected argument type, found {} '{}'", stringifyTok(t), i));
                return Package<array_t>::makeBroken(
                    fmt::format("failed to read argument type, expected {}, {}"
                        , stringifyTok(EToken::Scope), stringifyTok(EToken::TypeDyn)));
            }

            id_argtype = isDyn.assume_value().lexeme;
            std::invoke(modF, arr, std::pair{argName, LayoutTypeExpression{"?"}});
        } else id_argtype = argType.getResult().assume_value().toString();

        // if(!matchesReadResult(*buffered, std::array{EToken::Identifier, EToken::TypeDyn})) {
        //     auto&& [tok, idf] = *buffered;
        //     return Package<array_t>::makeBroken(
        //         fmt::format("Expected identifier, found {} '{}'", stringifyTok(tok), idf));
        // }

        std::invoke(modF, arr, std::pair{argName, id_argtype});

        // buffered = readBuffered();
        // if(!matchesReadResult(*buffered, Token::Comma)) {
        //     if(!matchesReadResult(*buffered, til)) {
        //         const auto& [tok, idf] = *buffered;
        //         return Package<array_t>::makeBroken(
        //             fmt::format("Expected comma or array terminator, found {} '{}'",
        //                 stringifyTok(tok), idf));
        //     }
        //     buffered.reset();
        //     return Package<array_t>{std::move(arr)};
        // }

        auto r_term2 = expectF(til).getResult();
        if(r_term2)
            return std::move(arr);

        auto r_comma = expectF(EToken::Comma).getResult();
        if(!r_comma) {
            return Package<array_t>::makeBroken(
                "Expected comma or array terminator", r_comma.assume_error().error);
        }
    }

    return Package<array_t>::makeBroken("Too much array elements");
}

template <template<typename...> typename V, typename VE,
    typename U, typename TE, typename TS, typename TT,
    // typename VE = std::conditional_t<std::is_same_v<TE, Token>, std::string, std::invoke_result_t<TE>>,
    typename = std::enable_if_t<
        std::conjunction_v<
            std::is_default_constructible<V<VE>>,
            std::disjunction<
                std::is_invocable<U, V<VE>&, const VE&>,
                std::is_invocable<U, V<VE>&, VE&&>> >,
            std::conjunction<
                std::negation<std::is_function<std::decay_t<TE>>>,
                std::negation<std::is_function<std::decay_t<TS>>>,
                std::negation<std::is_function<std::decay_t<TT>>> >
            >>
Package<V<VE>>
parseArray(TE&& getElem, TS&& getSepr, TT&& getTerm, U&& appendF) noexcept {
    using array_t = V<VE>;
    using read_result_t = std::invoke_result_t<TE&>; // std::pair<Token, std::string>

    // static_assert(rcx::callable::CallableObj<TE>);
    // static_assert(std::is_invocable<std::decay_t<TS>>::value); // false
    // static_assert(std::is_invocable<std::decay_t<TT>>::value); // false

    array_t arr{};

    for(unsigned i = 0; i < 1024; ++i) {
        auto&& isTerm = getTerm();
        // if(matchesReadResult(got, termType))
        if(isTerm)
            return Package<array_t>(std::move(arr));

        auto&& isElem = getElem();
        auto r_elem = isElem.getResult();
        if(!r_elem)
        /*if(!matchesReadResult(got, elemTF))*/ {
            return Package<array_t>::makeBroken(
                "Expected array element", r_elem.assume_error());
        }
        // NOTE: Token abandoned
        std::invoke(appendF, arr, r_elem.assume_value());

        // got = std::invoke(getF);
        auto&& isTerm1 = getTerm();
        // if(matchesReadResult(got, termType))
        if(isTerm1)
            return Package<array_t>(std::move(arr));

        auto&& isSepr = getSepr();
        if(!isSepr) {
            auto r_sepr = isSepr.getResult();
            return Package<array_t>::makeBroken("Expected array separator", r_sepr.assume_error());
        }
        // if(!matchesReadResult(got, seprType)) {
        //     const auto& [tok, idf] = got;
        //     spdlog::debug("Expected array separator, found {} '{}'",
        //             stringifyTok(tok), idf);
        //     return Package<array_t>::makeBroken(
        //         fmt::format("Expected array separator, found {} '{}'",
        //             stringifyTok(tok), idf));
        // }
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
Package<std::string>
parseName(T&& readF, U&& expectF) noexcept {

    auto r_name = expectF(EToken::Identifier).getResult();
    if(!r_name) return "Expected identifier";
    auto name = r_name.assume_value().lexeme;

    auto left_brace = expectF(std::pair{EToken::Angles, '<'});
    if(!left_brace)
        return Package<std::string>{std::move(name)};

    auto pkg_inner = parseName(std::forward<T>(readF), std::forward<U>(expectF));
    if(!pkg_inner) {
        auto right_brace = expectF(std::pair{EToken::Angles, '>'});
        if(!right_brace) return Package<std::string>::makeBroken("Expected '>'");
        return Package<std::string>{std::move(name) + "<>"};
    }

    auto r_inner = pkg_inner.getResult();

    auto right_brace = expectF(std::pair{EToken::Angles, '>'});
    if(!right_brace) return Package<std::string>::makeBroken("Expected '>'");

    return Package<std::string>{std::move(name) + r_inner.assume_value()};
}

template <typename T, typename U>
Package<LayoutObjectName>
parseNames(T&& readF, U&& expectF) noexcept {
    // TODO: make it capable parsing `some::name::space::Type`

    std::vector<std::string> full_type{};

    auto next1 = parseName(std::forward<T>(readF), std::forward<U>(expectF));
    if(!next1) return Package<LayoutObjectName>::makeBroken("Expected either namespace or type");

    full_type.push_back(next1.getResult().assume_value());

    for(auto i = 0; i< 1024; ++i) {
        auto next2 = expectF(EToken::Scope);
        if(!next2) {
            LayoutObjectName obj{std::move(full_type)};
            return Package<LayoutObjectName>{std::move(obj)};
            // return Package<ExpressionContext>{std::move(full_type)}; // FIX: vector into context
        }

        auto next3 = parseName(std::forward<T>(readF), std::forward<U>(expectF)); // expectF(Token::Identifier);
        if(!next3) return Package<LayoutObjectName>::makeBroken("Expected either namespace or type");
        
        full_type.push_back(std::move(next3.getResult().value()));
    }

    LayoutObjectName obj{std::move(full_type)};
    return Package<LayoutObjectName>{std::move(obj)};
}

const auto END = Package<LayoutValueExpression>::makeBroken("End");

template <typename T, typename U, typename TT>
Package<LayoutValueExpression>
parseExpression(T&& readF, U&& expectF, ctx::LocalSpaceContext& sc, TT&& termType) noexcept {

    auto is_term = expectF(std::forward<TT>(termType));
    if(is_term)
        return Package<LayoutValueExpression>{};

    auto is_not = expectF(EToken::Not);
    if(is_not) {
        BOOST_OUTCOME_TRY(auto v_entity
            , parseExpression(std::forward<T>(readF), std::forward<U>(expectF), sc, std::forward<TT>(termType)).getResult()
        );

        // [](LayoutCall c) { return LayoutBuilder::opNegate(std::move(c)); }
        auto e_negate = LayoutBuilder::opNegate(std::move(v_entity));
        return Package<LayoutValueExpression>{std::move(e_negate)};
    }

    auto is_ret = expectF(EToken::KeyReturn);
    if(is_ret) {
        auto pkg_entity = parseNames(std::forward<T>(readF), std::forward<U>(expectF));
        if(pkg_entity) {
            auto r_entity = pkg_entity.getResult();
            LayoutValueExpression inner_expr{LayoutBuilder::refer(r_entity.assume_value())};
            auto vRet = LayoutBuilder::buildReturn(std::move(inner_expr));
            LayoutValueExpression lve { std::move(vRet) };
            return std::move(lve);
        }

        auto pkg_expr = parseExpression(std::forward<T>(readF), std::forward<U>(expectF), sc, std::forward<TT>(termType));
        auto r_expr = pkg_expr.getResult();
        if(r_expr) {
            auto expr = r_expr.assume_value();

            // possible scenario: Expression Type in (Define/Initialize/Set/Call)
            // if(auto eType = expr.getEntityType();
            //     eType == ExpressionContext::ExpressionType::DefineVariable ||
            //     eType == ExpressionContext::ExpressionType::InitializeVariable)
            //     return Package<ExpressionContext>::makeBroken("Wrong expression in return statement");
            auto is_invalid = expr.visit([](const auto& a) -> std::optional<std::string> {
                // [](LayoutReturnVoid) {},
                // [](LayoutReturnValue) {}
                using a_type = std::decay<decltype(a)>;

                if constexpr (std::is_same_v<a_type, LayoutReturnVoid>)
                    return "invalid nested return expression";

                return {};
            });

            if(is_invalid) return Package<LayoutValueExpression>::makeBroken(*is_invalid);

            // Set/Call: define variable of it's output and return it
            auto r = LayoutBuilder::buildReturn(std::move(expr));
            return Package<LayoutValueExpression>{std::move(r)};
        }

        // return;
        auto r = LayoutBuilder::buildReturn();
        return Package<LayoutValueExpression>{std::move(r)};
    }

    auto pkg_name = parseNames(std::forward<T>(readF), std::forward<U>(expectF));
    auto r_name = pkg_name.getResult();

    if(!r_name) {
        auto is_term = expectF(std::forward<TT>(termType));
        if(is_term)
            return END;

        return Package<LayoutValueExpression>::makeBroken("Expected expression");
    }
    auto obj = r_name.assume_value();
    auto idf = obj.toString();

    // Scenario 1: Variable definition
    auto is_tp_def = expectF(EToken::Type);

    if(is_tp_def) {
        // define or define&init
        // string:(possibly::name_space::)type_name (= value_or_expression)
        auto pkg_type = parseNames(std::forward<T>(readF), std::forward<U>(expectF));
        auto r_type = pkg_type.getResult();
        if(!r_type)
            return Package<LayoutValueExpression>::makeBroken("Failed to parse variable type definition"
                , r_type.assume_error());

        auto oType = r_type.assume_value();
        auto eType = LayoutBuilder::refer(oType);

        auto is_set = expectF(EToken::VSet);
        if(!is_set) {
            auto def = LayoutBuilder::opSet(obj, std::move(eType));
            return Package<LayoutValueExpression>{LayoutValueExpression(std::move(def))};
        }

        auto value = parseExpression(std::forward<T>(readF), std::forward<U>(expectF), sc, std::forward<TT>(termType));
        auto r_value = value.getResult();
        auto ec = LayoutBuilder::defineVariable(obj, std::move(eType), r_value.assume_value());
        auto lv = LayoutValueExpression{std::move(ec)};
        return Package<LayoutValueExpression>{std::move(lv)};
    }

    // Scenario 2: Variable operation
    auto is_set = expectF(EToken::VSet);
    if(is_set) {
        auto pkg_value = parseExpression(std::forward<T>(readF), std::forward<U>(expectF), sc, std::forward<TT>(termType));
        auto r_value = pkg_value.getResult();
        auto ec = LayoutBuilder::opSet(obj, r_value.assume_value());
        return Package<LayoutValueExpression>{std::move(ec)};
    }

    // Scenario 3: function call
spdlog::debug("AA");
    auto op_l = expectF(std::pair{EToken::Parentheses, '('});
spdlog::debug("BB");
    if(!op_l) {
        // Scenario 4: Variable/Function refer/address
        auto is_ref_of = expectF(std::pair{EToken::Identifier, '.'});
spdlog::debug("REF {}", idf);
        if(is_ref_of) {
            auto p_expr_op = parseExpression(std::forward<T>(readF), std::forward<U>(expectF), sc, std::forward<TT>(termType));
            auto r_expr_op = p_expr_op.getResult();
            if(!r_expr_op)
                return Package<LayoutValueExpression>::makeBroken("Expected identifier", r_expr_op.assume_error());

            auto expr_of = r_expr_op.assume_value();
            auto tp_of = expr_of.visit([&obj](const auto& e) noexcept {
                using e_type = std::decay<decltype(e)>;

                if constexpr
                    (  std::is_same_v<e_type, LayoutRefer>
                    || std::is_same_v<e_type, LayoutCall> ) {
                    auto ec = LayoutBuilder::buildOf(obj, e);
                    return Package<LayoutValueExpression>{std::move(ec)};
                }

                return Package<LayoutValueExpression>::makeBroken("Expected identifier");
            });

            // if(tp_of == ExpressionContext::ExpressionType::Refer
            // || tp_of == ExpressionContext::ExpressionType::CallFunction ) {
            //     auto ec = ExpressionContextBuilder::buildOf(*obj, *expr_of);
            //     return Package<ExpressionContext>{std::move(ec)};
            // }

            // return Package<ExpressionContext>::makeBroken("Expected identifier");
        }

        auto is_term2 = expectF(std::forward<TT>(termType));
        auto r_term = is_term2.getResult();
        // .

        auto r = LayoutBuilder::refer(obj);
        auto lve = LayoutValueExpression{std::move(r)};
        return std::move(lve);
    }

    static const auto expectF_idf = [&expectF](auto&& e) noexcept -> Package<std::string> {
        auto&& o = std::invoke(expectF, std::forward<decltype(e)>(e));
        // auto r_o = o.getResult();
        // if(!r_o) {
        //     if constexpr(tupl::is_pair_v<std::decay_t<decltype(e)>>)
        //         return Package<std::string>::makeBroken(fmt::format("Expected {}", stringifyTok(static_cast<EToken>(e.first))));
        //     else
        //         return Package<std::string>::makeBroken(fmt::format("Expected {}", stringifyTok(static_cast<EToken>(e))));
        // }
        BOOST_OUTCOME_TRY(auto r_o, o.getResult());
        return r_o.lexeme;
    };

    auto args = parseArray<std::vector, LayoutValueExpression>(
        // std::bind_front(parseNames<T, U>, std::forward<T>(readF), std::forward<U>(expectF)),
        [&]() noexcept -> Package<LayoutValueExpression> {
            // return parseExpression(std::forward<decltype(readF)>(readF), std::forward<decltype(expectF)>(expectF), sc, std::pair{EToken::Parentheses, ')'});
            spdlog::debug("INNER: PARSE EXPRESSION");
            auto p_e = parseExpression(std::forward<decltype(readF)>(readF), std::forward<decltype(expectF)>(expectF), sc, std::pair{EToken::Parentheses, ')'});
            auto r_e = p_e.getResult();

            if(r_e) return r_e.assume_value();
            else spdlog::debug(r_e.assume_error());
            spdlog::debug("INNER; PARSE EXPRESSION");

            return r_e.assume_value();
        },
        std::bind(expectF_idf, EToken::Comma), std::bind(expectF_idf, std::pair{EToken::Parentheses, ')'}),
        [](auto& v, auto&& a) { v.push_back(std::forward<decltype(a)>(a)); } );

    auto r_args = args.getResult();

    if(r_args) {
spdlog::debug("EE");
        std::vector<LayoutValueExpression>&& vargs = std::move(r_args.assume_value());
        auto ec = LayoutCall(obj, std::move(vargs));
        return Package<LayoutValueExpression>{std::move(ec)};
    }
spdlog::debug("FF {}", r_args.assume_error());
    return Package<LayoutValueExpression>::makeBroken("Unexpected expression", r_args.assume_error());
}

template <typename T, typename U, typename TT>
Package<ctx::LocalSpaceContext>
parseExpressions(T&& readF, U&& expectF, ctx::LocalSpaceContext&, TT&& termType) noexcept {
    ctx::LocalSpaceContext spc_exprs;
    for(unsigned i = 0; i< 1024; ++i) {
        // TODO: specialize parseExpression package error type
        auto pkg_expr = parseExpression(std::forward<T>(readF), std::forward<U>(expectF), spc_exprs, std::forward<TT>(termType));
        
        spdlog::debug("0");
        if(!pkg_expr) {
            if(pkg_expr == END) break;
            
            spdlog::debug("1");
            auto r_expr = pkg_expr.getResult();
            spdlog::debug("2");
            return Package<ctx::LocalSpaceContext>::makeBroken("Failed to parse expression", r_expr.assume_error());
        }
        
        spdlog::debug("addDef 1");
        auto r_expr = pkg_expr.getResult();
        auto v_expr = r_expr.assume_value();
        spdlog::debug("addDef: {}", v_expr.toString());
        spc_exprs.addDef(v_expr);
        spdlog::debug("addDef 2");

        std::cin.get();
    }

    return Package<ctx::LocalSpaceContext>{std::move(spc_exprs)};
}

}

using namespace parser;
using annos_t = std::vector<std::string>;

template <typename T, typename U>
Package<LayoutEntityFunction>
parseFunction(T&& readF, U&& expectF, ctx::SpaceContext& ctx,
    std::optional<annos_t>&& annos = {}) noexcept {
    ;

    // TODO: annos

    {
        auto pkg_arg_open = expectF('(');
        if(!pkg_arg_open.hasValue()) {
            return "Expected function argument list";
        }
    }

    // auto pkg_arr = parseArray<std::vector, T>(
    //     std::forward<T>(readF), Token::Identifier, Token::Comma,
    //     std::pair{ Token::Braces, '}' },
    //     [](auto& v, auto&& a) { v.push_back(std::forward<decltype(a)>(a)); });

    // TODO: Dependent type system requires parse/computing Value expression on compile time
    // if support_dependent_type then parseDynamicArguments<..., std::pair<LayoutObjectName, `LayoutValueExpression`...
    auto pkg_args = parseDynamicArguments<std::vector, std::pair<LayoutObjectName, LayoutTypeExpression>>(
        std::forward<T>(readF), std::forward<U>(expectF),
        std::pair{ EToken::Parentheses, ')' },
        [](auto& v, auto&& a) { v.push_back(std::forward<decltype(a)>(a)); });

    if(!pkg_args.hasValue()) {
        // std::move(pkg_args).open();
        // return Package<LayoutEntityFunction>::makeBroken("Failed to parse function arguments");
        auto r_args = pkg_args.getResult();
        return Package<LayoutEntityFunction>::makeBroken("Failed to parse function arguments"
                , r_args.assume_error());
    }

    // auto args = *pkg_args; // vector<pair<string, string>>
    // auto vArgs = *pkg_args.open();
    auto vArgs = pkg_args.getResult().assume_value();
    std::vector<LayoutEntityVariable> args;
    std::transform(vArgs.begin(), vArgs.end(), std::back_inserter(args)
        , [](const std::pair<LayoutObjectName, LayoutTypeExpression>& p) noexcept {
            return LayoutEntityVariable{p.first, p.second};
        }
    );

    {
        auto pkg_arg_type_guide = expectF(EToken::TypeArrow);
        if(!pkg_arg_type_guide) {
            // std::move(pkg_arg_type_guide).open();
            // return Package<LayoutEntityFunction>::makeBroken("Failed to read function return type: expected '->'");
            auto r_atg = pkg_arg_type_guide.getResult();
            return Package<LayoutEntityFunction>::makeBroken("Failed to read function return type: expected '->'"
                    , r_atg.assume_error());
        }
    }

    auto pkg_ret_type = parseNames(std::forward<T>(readF), std::forward<U>(expectF)); // expectF(Token::Identifier);
    if(!pkg_ret_type) {
        // std::move(pkg_ret_type).open();
        // return Package<LayoutEntityFunction>::makeBroken("Failed to read function return type: expected return type");
        auto r_rt = pkg_ret_type.getResult();
        return Package<LayoutEntityFunction>::makeBroken("Failed to read function return type: expected return type"
            , r_rt.assume_error());
    }

    {
        auto pkg_body_open = expectF('{');
        if(!pkg_body_open) {
            // std::move(pkg_body_open).open();
            // return Package<LayoutEntityFunction>::makeBroken("Failed to read function: expected '{'");
            auto r_body = pkg_body_open.getResult();
            return Package<LayoutEntityFunction>::makeBroken("Failed to read function: expected '{'"
                    , r_body.assume_error());
        }
    }
    
    // TODO: specialized function for parsing function body is necessary
    ctx::LocalSpaceContext ns_space;
    // auto pkg_ns = parseGlobal(std::forward<T>(readF), std::forward<U>(expectF), ns_space);
    auto pkg_ex = parseExpressions(std::forward<T>(readF), std::forward<U>(expectF), ns_space, std::pair{ EToken::Braces, '}' });
    if(!pkg_ex) {
        // std::move(pkg_ex).open();
        // return Package<LayoutEntityFunction>::makeBroken("Failed to read function body");
        auto r_ex = pkg_ex.getResult();
        return Package<LayoutEntityFunction>::makeBroken("Failed to read function body", r_ex.assume_error());
    }

    // auto pkg_body_close = expectF('}');
    // if(!pkg_body_close) {
    //     return Package<ctx::FunctionContext>::makeBroken("Failed to read function: expected '}'");
    // }
    spdlog::debug("FUNCTION");

    // ctx::FunctionContext fc{"", *pkg_ret_type, std::move(args), std::move(ns_space), annos};
    // return Package<ctx::FunctionContext>{std::move(fc)};

    constexpr LayoutObjectName name{};
    if(annos->empty())
        return LayoutEntityFunction{std::move(name), pkg_ret_type.getResult().assume_value(), std::move(args)};

    LayoutEntityFunction fc{std::move(name), pkg_ret_type.getResult().assume_value(), std::move(args), std::move(annos)};
    return std::move(fc);
}

template <typename T, typename U>
Package<LayoutEntityClass>
parseStruct(T&& readF, U&& expectF, ctx::SpaceContext&) noexcept {
    return Package<LayoutEntityClass>::makeBroken("Struct parser is not implemented");
}

template <typename T, typename U>
Package<LayoutEntityFunction>
parseBinaryFunction(T&& readF, U&& expectF, ctx::SpaceContext&) noexcept {
    return Package<LayoutEntityFunction>::makeBroken("Binary function parser is not implemented");
}

template <typename T, typename U>
Package<annos_t>
parseAnnotation(T&& readF, U&& expectF, ctx::SpaceContext& ctx) noexcept {
    ;
    auto pkg_open = expectF('{');
    if(!pkg_open) {
        return Package<annos_t>::makeBroken("Expected annotation list");
    }

    const auto expectF_idf = [&expectF](auto&& e) noexcept -> Package<std::string> {
        auto&& o = std::invoke(expectF, std::forward<decltype(e)>(e));
        auto r_o = o.getResult();
        if(!o) {
            // auto err = std::move(o).open();
            // if(err) return Package<std::string>{err->second};

            if constexpr(tupl::is_pair_v<std::decay_t<decltype(e)>>)
                return Package<std::string>::makeBroken(fmt::format("Expected {}", stringifyTok(static_cast<EToken>(e.first))));
            else
                return Package<std::string>::makeBroken(fmt::format("Expected {}", stringifyTok(static_cast<EToken>(e))));
        }
        return r_o.assume_value().lexeme;
    };

    auto pkg_arr = parseArray<std::vector, std::string>(
        [&expectF_idf]() noexcept -> Package<std::string> {
            return std::invoke(std::forward<decltype(expectF_idf)>(expectF_idf), EToken::Identifier);
        }
        /*, std::forward<T>(readF), Token::Identifier*/
        , std::bind(expectF_idf, EToken::Comma)
        , std::bind(expectF_idf, std::pair{ EToken::Braces, '}' })
        , [](auto& v, auto&& a) noexcept { v.push_back(std::forward<decltype(a)>(a)); }
    );
    auto r_arr = pkg_arr.getResult();

    if(r_arr) {
        auto arr = r_arr.assume_value();
        spdlog::debug("Parsed {} attributes", arr.size());
    } else {
        spdlog::warn("Failed to parse array: {}", r_arr.assume_error());
        // 1: parsearray
    }

    return Package<annos_t>::cast<annos_t>(std::move(pkg_arr));
}

template <typename T, typename U>
Package<ctx::SpaceContext>
parseNamespace(T&& readF, U&& expectF, ctx::SpaceContext& ctxFrom) noexcept {

    auto pkg_name = expectF(EToken::Identifier);
    auto r_name = pkg_name.getResult();
    if(!pkg_name) {
        // return Package<ctx::SpaceContext>::makeBroken("Expected namespace name");
        return Package<ctx::SpaceContext>(*pkg_name.extract());
    }

    auto name = r_name.assume_value();
    spdlog::debug("namespace name = {}", name.lexeme);

    auto pkg_open = expectF('{');
    auto r_open = pkg_open.getResult();
    if(!pkg_open)
        return Package<ctx::SpaceContext>::makeBroken("Expected namespace body", r_open.assume_error());

    ctx::SpaceContext ns_space(name.lexeme, {});
    BOOST_OUTCOME_TRY(auto v_ns, parseGlobal(std::forward<T>(readF), std::forward<U>(expectF), ns_space).getResult());

    auto pkg_close = expectF('}');
    if(!pkg_close) {
        spdlog::debug("Expected closing bracket from namespace '{}'", name.lexeme);
        return Package<ctx::SpaceContext>::makeBroken("Expected namespace close");
    }

    ctxFrom.addDef(v_ns);
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
    // annos, name, meta variable, members&init values, methods
    // using cls_t = std::tuple<
    //     annos_t, std::string,
    //     metalist_t,
    //     std::vector<std::pair<std::string, expr_t>>,
    //     std::vector<fn_t> >;
    // using cls_t = LClass;

    using property_t = std::variant<annos_t
        , LayoutEntityAttributes
        , LayoutEntityFunction>;

    static_assert(std::is_copy_assignable<annos_t>::value);
    static_assert(std::is_copy_assignable<LayoutEntityAttributes>::value);
    static_assert(std::is_copy_assignable<LayoutEntityFunction>::value);
    static_assert(std::is_copy_assignable<std::optional<property_t>>::value);

    std::optional<property_t> parse_buf{};

    do {
        spdlog::debug("CONTEXT DUMP -- {");
        ctxFrom.dumpDef();
        spdlog::debug("CONTEXT DUMP -- }");

        auto [ tok, idf ] = readF();
        std::cout << idf << ": <" << stringifyTok(tok) << ">\n";
        if(EToken::__EOF == tok || EToken::__UNKNOWN == tok) break;

        // there's no known parsed metadata/property to continue.
        if (!parse_buf.has_value()) {
            // function parsing starts with the name
            if(tok == EToken::Identifier) {
                spdlog::debug("Try parsing function {} with no attributes", idf);
                auto pkg_fn = parseFunction(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                
                // auto fn = std::move(pkg_fn)();
                // if(fn.has_value()) ctxFrom.addDef((std::move(*fn)));
                // else spdlog::error("parseGlobal: Failed to open package, Function");

                BOOST_OUTCOME_TRY(auto fn, pkg_fn.getResult());
                ctxFrom.addDef(std::move(fn));

                ctxFrom.dumpDef();

                std::cin.get();
                continue;
            }

            if(tok == EToken::KeyStruct) {
                spdlog::debug("Try parsing struct {} with no attributes", idf);
                // parse_buf = cls_t({}, "", {}, {}, {});
                auto pkg_st = parseStruct(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);

                // auto st = std::move(pkg_st)();
                // if(st.has_value()) ctxFrom.addDef(std::make_shared<LayoutEntityClass>(std::move(*st)));
                // else spdlog::error("parseGlobal: Failed to open package, Struct");

                BOOST_OUTCOME_TRY(auto st, pkg_st.getResult());
                ctxFrom.addDef(std::make_shared<LayoutEntityClass>(std::move(st)));

		        continue;
            }

            // may be case of binary function definition
            if(idf == "(") {
                spdlog::debug("Try parsing binary function {} with no attributes", idf);
                auto pkg_bfn = parseBinaryFunction(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);

                // auto bfn = std::move(pkg_bfn)();
                // if(bfn.has_value()) ctxFrom.addDef(std::make_shared<LayoutEntityFunction>(std::move(*bfn)));
                // else spdlog::error("parseGlobal: Failed to open package, BinaryFunction");

                BOOST_OUTCOME_TRY(auto bfn, pkg_bfn.getResult());
                ctxFrom.addDef(std::make_shared<LayoutEntityFunction>(std::move(bfn)));

                continue;
            }

            if(tok == EToken::Annotation) {
                spdlog::debug("Try parsing attributes", idf);
                auto pkg_ann = parseAnnotation(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);
                spdlog::debug("---");
                // if(pkg_ann)
                    // parse_buf = (property_t)*pkg_ann.open();
                // else spdlog::error("parseGlobal: Failed to open package, annotation");
                // else std::move(pkg_ann).open();
                
                BOOST_OUTCOME_TRY(auto v_ann, pkg_ann.getResult());
                parse_buf = v_ann;

                continue;
            }

            if(tok == EToken::Namespace) {
                spdlog::debug("Parsing namespace {}", idf);
                auto pkg_ns = parseNamespace(
                    std::forward<T>(readF),
                    std::forward<U>(expectF), ctxFrom);

                // auto ns_ctx = std::move(pkg_ns).open();
                // if(ns_ctx.has_value()) ctxFrom.addDef(std::make_shared<ctx::SpaceContext>(std::move(*ns_ctx)));
                // else spdlog::error("parseGlobal: Failed to open package, Namespace");

                BOOST_OUTCOME_TRY(auto ns_ctx, pkg_ns.getResult());
                ctxFrom.addDef(std::make_shared<ctx::SpaceContext>(std::move(ns_ctx)));

// spdlog::debug("CTXFROM SIZE = {}", ctxFrom.dumpDef());

                continue;
            }

            spdlog::error("Unexpected {}, \'{}\'", stringifyTok(tok), idf);
            continue;
        }
        else {
            // function with annotation
            if(tok == EToken::Identifier) {
                spdlog::debug("Try parsing function {} with provided attributes", idf);

                bool is_anno = std::visit([](auto&& a) -> bool {
                    return std::is_same_v<std::decay_t<decltype(a)>, annos_t>;
                }, parse_buf.value());

                if (is_anno) {
                    auto&& a = std::get<annos_t>(parse_buf.value());
                    
                    auto pkg_fn = parseFunction(
                        std::forward<T>(readF),
                        std::forward<U>(expectF), ctxFrom, std::move(a));

                    // auto fn = std::move(pkg_fn)();
                    // if(fn.has_value()) ctxFrom.addDef((std::move(*fn)));
                    // else spdlog::error("parseGlobal: Failed to open package, Function");

                    BOOST_OUTCOME_TRY(auto fn, pkg_fn.getResult());
                    ctxFrom.addDef(std::move(fn));

                    ctxFrom.dumpDef();

                    parse_buf = {};
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
