#include "parser.hpp"

#include <algorithm>
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

Token
tokenizeIdf(std::string_view idf) noexcept {
    static
    const std::unordered_map<std::string_view, Token> tokMap = {
        { "namespace", Token::Namespace },
        { "(", Token::Parentheses },
        { ")", Token::Parentheses },
        { "{", Token::Braces },
        { "}", Token::Braces },
        { "<", Token::Angles },
        { ">", Token::Angles },
        { ",", Token::Comma },
        { ":", Token::Type },
        { "->", Token::TypeArrow },
        { "?", Token::TypeDyn },
        { "@", Token::Annotation },
        { "|", Token::Or },
        { "&", Token::And },
        { "case", Token::KeyCase },
        { "ret", Token::KeyReturn },
        { "=>", Token::KeyThen },
        { "struct", Token::KeyStruct }
    };
    auto it = tokMap.find(idf);
    return it != tokMap.end() ? it->second :
        std::all_of(idf.begin(), idf.end(), llvm::isDigit)
            ? Token::Digit : Token::Identifier;
}

std::string_view
stringifyTok(Token tok) noexcept {
    static
    const std::unordered_map<Token, std::string_view> nameMap {
        { Token::Digit, "Number" },
        { Token::Namespace, "namespace" },
        { Token::Parentheses, "parentheses" },
        { Token::Braces, "braces" },
        { Token::Angles, "angle braces" },
        { Token::Comma, "comma" },
        { Token::Type, "type" },
        { Token::TypeArrow, "arrow" },
        { Token::TypeDyn, "?" },
        { Token::Annotation, "annotation" },
        { Token::Or, "or" },
        { Token::And, "and" },
        { Token::KeyCase, "keyword case" },
        { Token::KeyReturn, "keyword return" },
        { Token::KeyThen, "=>" },
        { Token::KeyStruct, "structure" }
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

std::optional<std::pair<Token, std::string>>
SourceLexer::operator()(const auto& expected) noexcept {
    auto restore_buf = buf_;
    auto restore_pos = f_src_.tellg();
    auto got = (*this)();

    if(matchesReadResult(got, expected)) {
        return got;
    }

    buf_ = std::move(restore_buf);
    if(restore_pos != std::streampos(-1)) {
        f_src_.clear();
        f_src_.seekg(restore_pos);
    }

    return {};
}

std::pair<Token, std::string>
SourceLexer::operator()() noexcept {
// BGN:
    if (buf_.empty()) {
        if (f_src_.eof()) return { Token::__EOF, "" };
        std::getline(*reinterpret_cast<std::istream*>(&f_src_), buf_);
    }

    len_ = buf_.length();
    auto it = buf_.begin();
    unsigned cnt = 0;

    while(*(it + cnt) != EOF && llvm::isSpace(*(it + cnt)))
        if(++cnt == buf_.length()) return operator()(); // goto BGN;

    if(cnt) {
        buf_ = buf_.substr(cnt); // remove beginning whitespaces
        it = buf_.begin();
        // spdlog::debug("BUF: '{}', *it: '{}'", buf_, *it);
    }
    if(buf_.empty()) return operator()(); // goto BGN;
    
    if(buf_.front() == c_comment) {
        buf_.clear();
        return operator()(); // goto BGN;
    }

    if(*it == '-') cnt = 1;
    else cnt = 0;

    while(*(it + cnt) != EOF && llvm::isDigit(*(it + cnt))) ++cnt;
    if('.' == *(it + cnt)) {
        int cnt2 = cnt;
        auto a = buf_.substr(0, cnt);

        while(*(it + cnt2) != EOF && llvm::isDigit(*(it + cnt2))) ++cnt2;

        if(cnt == cnt2) {
            return { Token::Integer, a };
        }

        auto b = buf_.substr(cnt, cnt2);
        buf_ = buf_.substr(cnt);
        auto r = a + '.' + b;
        return { Token::Float, r };
    }
    if(cnt && llvm::isSpace(*(it + cnt))) {
        auto a = buf_.substr(0, cnt);
        buf_ = buf_.substr(cnt);
        return { Token::Integer, a };
    }

    cnt = 0;
    while(
        [](char a) noexcept {
            return llvm::isAlnum(a) || a == '_';
        }(*(it + cnt)) ) ++cnt;
    if(cnt) {
        auto a = buf_.substr(0, cnt);
        buf_ = buf_.substr(cnt);
        if("namespace" == a) return { Token::Namespace, a };
        if("ret" == a) return { Token::KeyReturn, a };
        return { Token::Identifier, a };
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
        return this->parseVStr(it, str, chHead, vch);
    };

    if(auto r = parseStr(buf_, '-', '>')) return { Token::TypeArrow, "->" };
    if(auto r = parseVStr.operator()<2>(buf_, '=', std::array<char, 2>{'>', '='}))
        return { Token::VRelations, *r };
    if(auto r = parseStr(buf_, '<', '=')) return { Token::VRelations, "<=" };
    if(auto r = parseStr(buf_, '>', '=')) return { Token::VRelations, ">=" };
    if(auto r = parseStr(buf_, ':', ':')) return { Token::Scope, "::" };

    std::string a; a.push_back(buf_.front());
    buf_ = buf_.substr(1);

    static std::unordered_map<std::string, Token> stot {
        { "(", Token::Parentheses },
        { ")", Token::Parentheses },
        { "{", Token::Braces },
        { "}", Token::Braces },
        { "<", Token::Angles },
        { ">", Token::Angles },
        { "=", Token::VSet },
        { ":", Token::Type },
        { "@", Token::Annotation },
        { "?", Token::TypeDyn },
        { ",", Token::Comma }
    };

    auto tok = stot[a];
    if(Token::__UNKNOWN != tok) return { tok, a };

    return { Token::__UNKNOWN, a };
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
        using ivk_rt = std::optional<std::pair<Token, std::string>>;
        using ivk_rt_vt = typename ivk_rt::value_type;

        auto parse_expect
        = [&f_idf](/*auto& from, */const auto& to) noexcept
        -> Package<ivk_rt_vt> {
            // using from_t = std::remove_reference_t<decltype(from)>;
            using to_t = std::remove_cv_t<std::remove_reference_t<decltype(to)>>;

            using ret_t = ivk_rt_vt; // std::invoke_result_t<decltype(f_idf)::operator(), std::string>;
            using ret_t0 = ret_t::first_type;  // Token
            using ret_t1 = ret_t::second_type; // string
            static_assert(std::is_same_v<ret_t0, Token>);
            // static_assert(std::is_same_v<ret_t1, from_t>, "`f_idf` should return pair which second element has the same type of `from`");

            auto got = f_idf(to);
            if(!got) {
                if constexpr (std::is_same_v<to_t, Token>) {
                    return Package<ret_t>::makeBroken(
                        fmt::format("Expected {}", stringifyTok(to)));
                }/* else if constexpr (std::is_convertible_v<to_t, char>) {
                    return Package<ret_t>::makeBroken(
                        fmt::format("Expected '{}'", static_cast<char>(to)));
                }*/ else if constexpr (std::is_convertible_v<to_t, std::string>) {
                    return Package<ret_t>::makeBroken(
                        fmt::format("Expected '{}'", std::string{to}));
                } else {
                    // spdlog::debug(":= {}", got);
                    return Package<ret_t>::makeBroken("Unexpected expectation type");
                }
            }

            return Package<ret_t>(std::move(*got));
        };

        auto f_expect
        = [&f_idf, expect = parse_expect](const auto& a) noexcept
         -> Package<
                typename
                std::invoke_result_t<
                    decltype(parse_expect),
                    decltype(a) >::content_type > {
            // suppose `a` be Token or char/string
            using out_t = std::invoke_result_t<decltype(parse_expect), decltype(a)>;
            using cont_t = typename out_t::content_type;
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
            if(tok == Token::Identifier) {
                parse_buf = fn_t({}, idf, {}, {}, {});
                continue;
            }

            if(tok == Token::KeyStruct) {
                parse_buf = cls_t({}, "", {}, {}, {});
		        continue;
            }

            // may be case of binary function definition
            if(idf == "(") {
                auto pkg = f_expect(Token::Identifier);
                if(!pkg.hasValue()) {
                    ;
                }
                auto pkg_op = std::move(pkg).open().value();
                auto name = pkg_op.second;

                parse_buf = fn_t({}, fmt::format("({})", name), {}, {}, {});
                continue;
            }

            if(tok == Token::Annotation) {
                parse_buf = annos_t{};
                continue;
            }

            if(tok == Token::Namespace) {
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
                    if(tok == Token::TypeArrow) {
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
