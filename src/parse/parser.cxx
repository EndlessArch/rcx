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
tokenizeIdf(std::string& idf) noexcept {
    static
    const std::unordered_map<std::string, Token> tokMap = {
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

std::string
stringifyTok(Token tok) noexcept {
    static
    const std::unordered_map<Token, std::string> nameMap {
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

template <typename F>
Package<metavars_t>
parseMetaVars(F& freader) noexcept {
    auto [tok, idf] = freader();
    // empty case
    if(idf == ">") return Package<metavars_t>(metavars_t{});

    // auto tok = tokenizeIdf(idf);
    
    if(tok != Token::Identifier) {
        return Package<metavars_t>::makeBroken( (std::string)
            fmt::format("Expected {}, found {}", stringifyTok(tok), idf));
    }

    std::pair<std::string, expr_t*> meta_pair{};

    return Package<metavars_t>::makeBroken("");
}

} // ns parser

using namespace parser;

Package<ctx::context_t>
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

    auto f_idf = [&f_src]() noexcept -> std::pair<Token, std::string> {
        static std::string buf;
        static unsigned len;
        static constexpr char c_comment = '#';
BGN:
        if (buf.empty()) {
            if (f_src.eof()) return { Token::__EOF, "" };
            std::getline(*reinterpret_cast<std::istream*>(&f_src), buf);
        }

        len = buf.length();
        auto it = buf.begin();
        unsigned cnt = 0;

        while(*(it + cnt) != EOF && llvm::isSpace(*(it + cnt)))
            if(++cnt == buf.length()) goto BGN;

        if(cnt) buf = buf.substr(cnt); // remove beginning whitespaces

        // spdlog::debug("nospace: {}", buf);
        if(buf.empty()) goto BGN;
        if(buf.front() == c_comment) {
            buf.clear();
            goto BGN;
        }

        if(*it == '-') cnt = 1;
        else cnt = 0;
    
        while(*(it + cnt) != EOF && llvm::isDigit(*(it + cnt))) ++cnt;
        if('.' == *(it + cnt)) {
            // possibly float
            int cnt2 = cnt;
            auto a = buf.substr(0, cnt);

            while(*(it + cnt2) != EOF && llvm::isDigit(*(it + cnt2))) ++cnt2;

            if(cnt == cnt2) {
                // or maybe throw error: unexpected '.'
                return { Token::Integer, a };
            }

            auto b = buf.substr(cnt, cnt2);
            buf = buf.substr(cnt);
            return { Token::Float, a + '.' + b };
        }
        if(cnt && llvm::isSpace(*(it + cnt))) {
            auto a = buf.substr(0, cnt);
            buf = buf.substr(cnt);
            return { Token::Integer, a };
        }

        cnt = 0;
        while(
            [](char a) noexcept {
                return llvm::isAlnum(a) || a == '_';
            }(*(it + cnt)) ) ++cnt;
        if(cnt) {
            // alnum identifier
            auto a = buf.substr(0, cnt);
            buf = buf.substr(cnt);
            if("namespace" == a) return { Token::Namespace, a };
            return { Token::Identifier, a };
        }

        auto parseStr = [&it](auto& str, char chHead, auto chNext) -> std::optional<std::string> {
            if(str.front() == chHead) {
                if(str.length()> 1 && str[1] == chNext) {
                    auto a = std::string(it, it+ 1);
                    str = str.substr(2);
                    return a;
                }
            }
            return {};
        };

        auto parseVStr = [&it]<std::size_t N>(auto& str, char chHead, std::array<char, N> vch) -> std::optional<std::string> {
            if(str.front() != chHead)  return {};
            if(str.length() <= 1) return {};

            for(auto c : vch) {
                if(c == str[1]) {
                    auto a = std::string(it, it+ 1);
                    str = str.substr(2);
                    return a;
                }
            }

            return {};
        };

        // special character
        if(auto r = parseStr(buf, '-', '>')) return { Token::TypeArrow, *r };
        // if(buf.front() == '-') {
        //     if(len> 1 && buf.at(1) == '>') {
        //         auto a = std::string(it, it + 1);
        //         buf = buf.substr(2);
        //         return a;
        //     }
        // }
        if(auto r = parseVStr.operator()<2>(buf, '=', {'>', '='})) return { Token::VRelations, *r };
        // if(buf.front() == '=') {
        //     if(len > 1) {
        //         if(auto ch = buf.at(1);
        //         ch == '>' || ch == '=') {
        //             auto a = std::string(it, it + 1);
        //             buf = buf.substr(2);
        //             return a;
        //         }
        //     }
        // }
        if(auto r = parseStr(buf, '<', '=')) return { Token::VRelations, *r };
        // if(buf.front() == '<') {
        //     if(len> 1 && buf.at(1) == '=') {
        //         auto  a = std::string(it, it + 1);
        //         buf = buf.substr(2);
        //         return a;
        //     }
        // }
        if(auto r = parseStr(buf, '>', '=')) return { Token::VRelations, *r };
        // if(buf.front() == '>') {
        //     if(len> 1 && buf.at(1) == '=') {
        //         auto a = std::string(it, it + 1);
        //         buf = buf.substr(2);
        //         return a;
        //     }
        // }

        if(auto r = parseStr(buf, ':', ':')) return { Token::Scope, *r };

        std::string a; a.push_back(buf.front());
        // spdlog::debug("a: {}", a.c_str());
        buf = buf.substr(1);
        
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
            { "?", Token::TypeDyn }
        };

        // if(auto it = stot.find(a); it != stot.end()) return { it->second, a };
        
        auto tok = stot[a];
        if(Token::__UNKNOWN != tok) return { tok, a };
        
        return { Token::Identifier, a };
    };

    // NOTE: identifier does not start with special character.
    auto cur_ctx =
        ctx::SpaceContext::null().setName("$global_namespace");

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
        static const auto parse_expect
        = [&f_idf](/*auto& from, */const auto& to) noexcept
        -> Package<std::invoke_result_t<decltype(f_idf)>> {
            // using from_t = std::remove_reference_t<decltype(from)>;
            using to_t = std::remove_cv_t<std::remove_reference_t<decltype(to)>>;

            using ret_t = std::invoke_result_t<decltype(f_idf)>;
            using ret_t0 = ret_t::first_type;  // Token
            using ret_t1 = ret_t::second_type; // string
            static_assert(std::is_same_v<ret_t0, Token>);
            // static_assert(std::is_same_v<ret_t1, from_t>, "`f_idf` should return pair which second element has the same type of `from`");

            auto [tk, idf] = f_idf();
            // from = idf;
            if constexpr (std::is_same_v<to_t, Token>) {
                if(tk != to) {
                    return Package<ret_t>::makeBroken(
                        fmt::format("Expected {}, found {}", stringifyTok(to), stringifyTok(tk)));
                }
            } else {
                // assuming `to` to be string or something
                // if constexpr (std::is_convertible_v<to_t, from_t>) {
                //     if constexpr (ctr_from<from_t>(from) != to)
                //         spdlog::error("Expected \'{}\', found \'{}\'", from, to);
                // }
            }
            // return from;
            return Package<ret_t>(ret_t{tk, idf});
        };

        static const auto f_expect
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

        // there's no known parsed metadata/property to continue.
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
                auto pkg_op = pkg.open().value();
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

    return Package<ctx::context_t>(std::move(cur_ctx));
    // return Package<ctx::context_t>::makeBroken("Failed task.");
}

Package<ctx::context_t>
parseModule(void) noexcept {
    return Package<ctx::context_t>::makeBroken("Failed to parse module.");
}

NSRCXEND
