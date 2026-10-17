#include "parser.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>

#include <conv/Lazy.hpp>
#include <conv/Tuples.hpp>
#include <parse/AST/AST.hpp>
#include <parse/CTX/Context.hpp>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringExtras.h> // isalnum
#include <llvm/ADT/Triple.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

NSRCXBGN

namespace parser {

EToken
tokenizeIdf(std::string_view idf) noexcept {
    static
    const std::unordered_map<std::string_view, EToken> tokMap = {
        { "namespace", EToken::Namespace },
        { "(", EToken::Parentheses },
        { ")", EToken::Parentheses },
        { "{", EToken::Braces },
        { "}", EToken::Braces },
        { "<", EToken::Angles },
        { ">", EToken::Angles },
        { ",", EToken::Comma },
        { ":", EToken::Type },
        { "::", EToken::Scope },
        { "->", EToken::TypeArrow },
        { "!", EToken::Not },
        { "?", EToken::TypeDyn },
        { "@", EToken::Annotation },
        { "|", EToken::Or },
        { "&", EToken::And },
        { "case", EToken::KeyCase },
        { "ret", EToken::KeyReturn },
        { "=>", EToken::KeyThen },
        { "struct", EToken::KeyStruct }
    };
    auto it = tokMap.find(idf);
    return it != tokMap.end() ? it->second :
        std::all_of(idf.begin(), idf.end(), llvm::isDigit)
            ? EToken::Digit : EToken::Identifier;
}

std::string_view
stringifyTok(EToken tok) noexcept {
    static
    const std::unordered_map<EToken, std::string_view> nameMap {
        { EToken::Digit, "Number" },
        { EToken::Namespace, "namespace" },
        { EToken::Parentheses, "parentheses" },
        { EToken::Braces, "braces" },
        { EToken::Angles, "angle braces" },
        { EToken::Comma, "comma" },
        { EToken::Type, "type" },
        { EToken::TypeArrow, "arrow" },
        { EToken::Scope, "scope" },
        { EToken::TypeDyn, "?" },
        { EToken::Annotation, "annotation" },
        { EToken::Not, "!" },
        { EToken::Or, "or" },
        { EToken::And, "and" },
        { EToken::VSet, "=" },
        { EToken::VRelations, "comparison operator" },
        { EToken::KeyCase, "keyword case" },
        { EToken::KeyReturn, "keyword return" },
        { EToken::KeyThen, "=>" },
        { EToken::KeyStruct, "structure" }
    };
    // spdlog::debug("Token: {}", (int)tok);
    auto a = nameMap.find(tok);
    return a == nameMap.end() ? "identifier" : nameMap.at(tok);
}

// template <typename F>
// Package<metavars_t>
// parseMetaVars(F& freader) noexcept {
//     auto [tok, idf] = freader();
//     // empty case
//     if(idf == ">") return Package<metavars_t>(metavars_t{});

//     // auto tok = tokenizeIdf(idf);
    
//     if(tok != Token::Identifier) {
//         return Package<metavars_t>::makeBroken( (std::string)
//             fmt::format("Expected {}, found {}", stringifyTok(tok), idf));
//     }

//     std::pair<std::string, expr_t*> meta_pair{};

//     return Package<metavars_t>::makeBroken("");
// }
 
// bool
// SourceLexer::matches(const std::pair<Token, std::string>& got, const auto& expected) const noexcept {
//     using expected_t = std::remove_cv_t<std::remove_reference_t<decltype(expected)>>;

//     const auto& [tok, idf] = got;
//     if constexpr(std::is_same_v<expected_t, Token>) {
//         return tok == expected;
//     } else if constexpr(std::is_convertible_v<expected_t, char>) {
//         return idf.size() == 1 && idf.front() == static_cast<char>(expected);
//     } else if constexpr(std::is_convertible_v<expected_t, std::string>) {
//         return idf == std::string{expected};
//     } else {
//         return false;
//     }
// }

// FIX: Not synchronized with read <=> operator()()
Package<Token, EToken>
SourceLexer::operator()(const auto& expected) noexcept {
    std::string restore_buf = buf_;
    setPos();

    auto got = (*this)();

    if(matchesReadResult(got, expected)) {
        // setPos();
        return got;
    }
    else spdlog::debug("NNN");

    spdlog::debug("THIS: '{}': {} {}", got.lexeme, stringifyTok(got.kind), (int)got.kind);
    // if constexpr (std::is_same_v<std::decay_t<decltype(expected)>, Token>)
    //     spdlog::debug("WANT: {}", stringifyTok(expected));

    buf_ = std::move(restore_buf);
    const auto lastPos = getPos();
    if(lastPos != std::streampos(-1)) {
        f_src_.clear();
        f_src_.seekg(lastPos);
    }

    return got.kind;
}

Token
SourceLexer::operator()() noexcept {
// BGN:
    // refill buffer
    if (buf_.empty()) {
        if (f_src_.eof()) return Token { EToken::__EOF, "" };
        std::getline(*reinterpret_cast<std::istream*>(&f_src_), buf_);
    }

    len_ = buf_.length();
    auto it = buf_.begin();
    unsigned cnt = 0;

    // filter whitespace
    while(*(it + cnt) != EOF && llvm::isSpace(*(it + cnt)))
        if(++cnt == buf_.length()) return operator()(); // goto BGN;

    if(cnt) {
        buf_ = buf_.substr(cnt); // remove beginning whitespaces
        it = buf_.begin();
        // spdlog::debug("BUF: '{}', *it: '{}'", buf_, *it);
    }
    if(buf_.empty()) return operator()(); // goto BGN;
    
    // filter comment
    if(buf_.front() == c_comment) {
        buf_.clear();
        return operator()(); // goto BGN;
    }

    // read (-)|digit/float
    if(*it == '-') cnt = 1;
    else cnt = 0;

    while(*(it + cnt) != EOF && llvm::isDigit(*(it + cnt))) ++cnt;
    if('.' == *(it + cnt)) {
        int cnt2 = cnt;
        auto a = buf_.substr(0, cnt);

        while(*(it + cnt2) != EOF && llvm::isDigit(*(it + cnt2))) ++cnt2;

        if(cnt == cnt2) {
            if(cnt) {
                buf_ = buf_.substr(cnt);
                return Token { EToken::Integer, a };
            }
            buf_ = buf_.substr(1);
            return Token { EToken::Identifier, "." }; // '.'
        }

        auto b = buf_.substr(cnt, cnt2);
        buf_ = buf_.substr(cnt);
        auto r = a + '.' + b;
        return Token { EToken::Float, r };
    }
    if(cnt && llvm::isSpace(*(it + cnt))) {
        auto a = buf_.substr(0, cnt);
        buf_ = buf_.substr(cnt);
        return Token { EToken::Integer, a };
    }

    cnt = 0;
    while(
        [](char a) noexcept {
            return llvm::isAlnum(a) || a == '_';
        }(*(it + cnt)) ) ++cnt;
    if(cnt) {
        auto a = buf_.substr(0, cnt);
        buf_ = buf_.substr(cnt);
        if("namespace" == a) return Token { EToken::Namespace, a };
        if("ret" == a) return Token { EToken::KeyReturn, a };
        return Token { EToken::Identifier, a };
    }

    auto parseStr = [this, &it](auto& str, char chHead, auto chNext) -> std::optional<std::string> {
        // if(str.front() == chHead) {
        //     if(str.length()> 1 && str[1] == chNext) {
        //         auto a = std::string(it, it+ 1);
        //         str = str.substr(2);
        //         return a;
        //     }
        // }
        // return {};
        return this->parseStr(it, str, chHead, chNext);
    };

    auto parseVStr = [this, &it]<std::size_t N>(auto& str, char chHead, std::array<char, N> vch) -> std::optional<std::string> {
        // if(str.front() != chHead)  return {};
        // if(str.length() <= 1) return {};

        // for(auto c : vch) {
        //     if(c == str[1]) {
        //         auto a = std::string(it, it+ 1);
        //         str = str.substr(2);
        //         return a;
        //     }
        // }

        // return {};
        return this->parseVStr<N>(it, str, chHead, vch);
    };

    if(auto r = parseStr(buf_, '-', '>')) return Token { EToken::TypeArrow, "->" };
    if(auto r = parseVStr.operator()<2>(buf_, '=', std::array<char, 2>{'>', '='}))
        return Token { EToken::VRelations, *r };
    if(auto r = parseStr(buf_, '<', '=')) return Token { EToken::VRelations, "<=" };
    if(auto r = parseStr(buf_, '>', '=')) return Token { EToken::VRelations, ">=" };
    if(auto r = parseStr(buf_, ':', ':')) return Token { EToken::Scope, "::" };

    std::string a; a.push_back(buf_.front());
    buf_ = buf_.substr(1);

    static std::unordered_map<std::string, EToken> stot {
        { "(", EToken::Parentheses },
        { ")", EToken::Parentheses },
        { "{", EToken::Braces },
        { "}", EToken::Braces },
        { "<", EToken::Angles },
        { ">", EToken::Angles },
        { "=", EToken::VSet },
        { ":", EToken::Type },
        { "@", EToken::Annotation },
        { "?", EToken::TypeDyn },
        { "!", EToken::Not },
        { ",", EToken::Comma }
    };

    auto tok = stot[a];
    if(EToken::__UNKNOWN != tok) return Token { tok, a };

    return Token { EToken::__UNKNOWN, a };
}

} // ns parser

using namespace parser;

Package<ctx::SpaceContext>
parseStart(argparse::ArgumentParser && optMap) noexcept {
    ;

    std::fstream f_src;
    Lazy<std::fstream, std::string, std::ios_base::openmode> f_out;

    {
        auto sourceName = optMap.get<std::string>("-s");
        auto destName = optMap.get<std::string>("-o");

        f_src = std::fstream(sourceName, std::ios_base::in);
        f_out.setArgs(destName, std::ios_base::out);
    }

    SourceLexer f_idf{f_src};

    // NOTE: identifier does not start with special character.
    auto cur_ctx =
        ctx::SpaceContext().setName("$global_namespace");

    using type_t = ast::Type;
    using expr_t = parser::expr_t;

    using annos_t = std::vector<std::string>;
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
        // using ivk_rt = std::invoke_result_t<decltype(&SourceLexer::operator()), SourceLexer, const std::string&>;
        using ivk_rt = Package<Token, TokenErrC>;

        auto parse_expect
        = [&f_idf](/*auto& from, */const auto& to) noexcept
        -> ivk_rt {
            // using from_t = std::remove_reference_t<decltype(from)>;
            using to_t = std::remove_cv_t<std::remove_reference_t<decltype(to)>>;
            using err_value_t = typename ivk_rt::failure_type;

            static auto make_error_sv = [&to](const auto& result) noexcept -> std::string {
                if constexpr (std::is_same_v<to_t, Token>) {
                    return fmt::format("Expected {}, found", stringifyTok(static_cast<Token>(to).kind));
                }

                if constexpr (std::is_same_v<to_t, EToken>) {
                    return fmt::format("Expected {}, found", stringifyTok(static_cast<EToken>(to)));
                }/* else if constexpr (std::is_convertible_v<to_t, char>) {
                    return Package<ret_t>::makeBroken(
                        fmt::format("Expected '{}'", static_cast<char>(to)));
                }*/ else if constexpr (std::is_convertible_v<to_t, std::string>) {
                    return fmt::format("Expected '{}', found", std::string{to});
                } else {
                    // spdlog::debug(":= {}", got);
                    return fmt::format("Unexpected expectation type");
                }
            };

            auto got = f_idf(to);
            auto result = got.getResult();

            if(!result) {
                auto kind = result.assume_error();
                auto err_sv = fmt::format("{} '{}'", make_error_sv(result), stringifyTok(kind));
                return TokenErrC { kind, err_sv };
            }

            return std::move(result.assume_value());
        };

        auto f_expect
        = [&f_idf, expect = parse_expect](const auto& a) noexcept
         -> std::invoke_result_t<
                decltype(parse_expect),
                decltype(a) >
        {
            // suppose `a` be Token or char/string
            using out_t = std::invoke_result_t<decltype(parse_expect), decltype(a)>;
            using cont_t = typename out_t::success_type;
            // static_assert(std::is_same_v<cont_t, std::pair<Token, std::string>>, "Not same");

            using a_nr_t = std::remove_reference_t<decltype(a)>;
            using a_t = std::remove_cv_t<a_nr_t>;
            // using buf_t = std::conditional_t<std::is_same_v<Token, a_t>, std::string, a_t>;
            // static_assert(std::is_default_constructible_v<buf_t>, "`buf_t` is not default constructible");

            auto pkg = expect(a);

            return pkg;
            // return Package<buf_t>::transform(
            //     std::move(pkg),
            //     [](cont_t&& a) { return std::forward<buf_t>(a.second); });
        };

        auto r = parseGlobal(f_idf, f_expect, cur_ctx);
        std::cout << "About to return\n";
        return r;
        
        auto [ tok, idf ] = f_idf();
        if(idf.empty()) break;

        // no known parsed metadata/property to continue.
        if (!parse_buf.has_value()) {
            // function parsing starts with the name
            if(tok == EToken::Identifier) {
                parse_buf = fn_t({}, idf, {}, {}, {});
                continue;
            }

            if(tok == EToken::KeyStruct) {
                parse_buf = cls_t({}, "", {}, {}, {});
		        continue;
            }

            // may be case of binary function definition
            if(idf == "(") {
                auto pkg = f_expect(EToken::Identifier);
                if(!pkg.hasValue()) {
                    return "Expected binary function";
                }
                BOOST_OUTCOME_TRY(auto pkg_op, pkg.getResult());
                // auto pkg_op = std::move(pkg).open().value();
                auto name = pkg_op.lexeme;

                parse_buf = fn_t({}, fmt::format("({})", name), {}, {}, {});
                continue;
            }

            if(tok == EToken::Annotation) {
                parse_buf = annos_t{};
                continue;
            }

            if(tok == EToken::Namespace) {
                auto ns_ctx = parseNamespace(f_idf, f_expect, cur_ctx);

                // ctx::SpaceContext a_space(cur_ctx, parse_expect(idf, Token::Identifier), {});
                // cur_ctx.addDef(a_space);

                // parse_expect(idf, '{');

                continue;
            }

            spdlog::error("Unexpected {}, \'{}\'", stringifyTok(tok), idf);
            continue;
        } else { // at least annos are parsed
            std::visit([&idf, tok, &f_idf](auto&& arg) -> void {
                using arg_t = std::remove_reference_t<decltype(arg)>;
                if constexpr (
                    std::is_same_v<fn_t, arg_t>
                ) {
                    // auto& fannos = std::get<0>(arg);
                    // auto& fname = std::get<1>(arg);
                    // auto& fmetas = std::get<2>(arg);
                    // auto& fargs = std::get<3>(arg);
                    // auto& fbody = std::get<4>(arg);
                    if(idf == "<") {
                        // parse metalist
                        parser::parseMetaVars(f_idf);
                    }
                    if(idf == "(") {
                        // parse arguments
                    }
                    if(tok == EToken::TypeArrow) {
                        // parse return type
                    }
                    if(idf == "{") {
                        // parse expressions
                    }
                }
                if constexpr (
                    std::is_same_v<cls_t, arg_t>
                ) {
                    ;
                }
                // annos only
            }, parse_buf.value());
        }
    } while(1);

    return Package<ctx::SpaceContext>(std::move(cur_ctx));
    // return Package<ctx::context_t>::makeBroken("Failed task.");
}

Package<ctx::SpaceContext>
parseModule(void) noexcept {
    return Package<ctx::SpaceContext>::makeBroken("Failed to parse module.");
}

NSRCXEND
