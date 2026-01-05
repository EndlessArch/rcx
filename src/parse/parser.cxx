#include "parser.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>

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
        const std::map<std::string, Token> tokMap = {
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
    const std::map<Token, std::string> nameMap {
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
    std::string idf = freader();
    if(idf == ">") return Package<metavars_t>(metavars_t{});

    auto tok = tokenizeIdf(idf);
    
    if(tok != Token::Identifier) {
        return Package<metavars_t>::makeBroken(
            fmt::format("Expected {}, found {}", stringifyTok(tok), idf));
    }

    std::pair<std::string, expr_t*> meta_pair{};

    return Package<metavars_t>::makeBroken("");
}

} // ns parser

Package<ctx::context_t>
parseStart(argparse::ArgumentParser && optMap) noexcept {
    ;

    std::fstream f_src, f_out;

    {
        auto sourceName = optMap.get<std::string>("-s");
        auto destName = optMap.get<std::string>("-o");

        f_src = std::fstream(sourceName, std::ios_base::in);
        f_out = std::fstream(destName, std::ios_base::out);
    }

    auto f_idf = [&f_src]() -> std::string {
        static std::string buf;
        static constexpr char c_comment = '#';
BGN:
        if (buf.empty()) {
            if (f_src.eof()) return "";
            std::getline(*reinterpret_cast<std::istream*>(&f_src), buf);
        }
        auto it = buf.begin();
        unsigned cnt = 0;
        while(*(it + cnt) != EOF && llvm::isSpace(*(it + cnt))) ++cnt;
        if(cnt) buf = buf.substr(cnt); // remove beginning whitespaces
        // spdlog::debug("nospace: {}", buf);
        if(buf.empty()) goto BGN;
        if(buf.front() == c_comment) {
            buf.clear();
            goto BGN;
        }

        cnt = 0;
        while(llvm::isDigit(*(it + cnt))) ++cnt;
        if(cnt && llvm::isSpace(*(it + cnt))) {
            auto a = buf.substr(0, cnt);
            spdlog::debug("::{}", a);
            buf = buf.substr(cnt);
            return a;
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
            return a;
        }
        // special character
        if(buf.front() == '-') {
            if(buf.at(1) == '>') {
                auto a = std::string(it, it + 1);
                buf = buf.substr(2);
                return a;
            }
        }
        if(buf.front() == '=') {
            if(auto ch = buf.at(1);
            ch == '>' || ch == '=') {
                auto a = std::string(it, it + 1);
                buf = buf.substr(2);
                return a;
            }
        }
        if(buf.front() == '<') {
            if(buf.at(1) == '=') {
                auto  a = std::string(it, it + 1);
                buf = buf.substr(2);
                return a;
            }
        }
        if(buf.front() == '>') {
            if(buf.at(1) == '=') {
                auto a = std::string(it, it + 1);
                buf = buf.substr(2);
                return a;
            }
        }
        std::string a; a.push_back(buf.front());
        buf = buf.substr(1);
        // spdlog::debug("a: {}", a.c_str());
        return a;
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

    //    using namespace parser;

    do {
        auto idf = f_idf();
        // enum class ParsingType {
        //     Unknown,
        //     Class, Function,
        //     CTFunction
        // } target_type = ParsingType::Unknown;

#define handle_unexpected(tok, idf) \
{ spdlog::error("Unexpected " + stringifyTok((tok)) + ", \'" + (idf) + "\'"); \
continue; }

        if(idf.empty()) break;

	using namespace parser;

        auto tok = tokenizeIdf(idf);

        static auto parse_expect
        = [&f_idf](auto& from, const auto& to) noexcept {
            using to_t = std::remove_cv_t<std::remove_reference_t<decltype(to)>>;
            from = f_idf();
            if constexpr (std::is_same_v<to_t, Token>) {
                if(auto tk = tokenizeIdf(from); tk != to)
                    spdlog::error("Expected {}, found {}", stringifyTok(to), stringifyTok(tk));
            } else
                if(from != to) spdlog::error("Expected \'{}\', found \'{}\'", from, to);
            return from;
        };

        if (!parse_buf.has_value()) {
            if(tok == Token::Annotation) {
                parse_buf = annos_t{};
                continue;
            }
            // function parsing starts with the name,
            // otherwise, {}.
            if(tok == Token::Identifier) {
                parse_buf = fn_t({}, idf, {}, {}, {});
                continue;
            }
            if(idf == "(") {
                // parse binary function
                parse_buf = fn_t({}, fmt::format("({})", parse_expect(idf, Token::Identifier)), {}, {}, {});
                continue;
            }
            if(tok == Token::KeyStruct) {
                parse_buf = cls_t({}, "", {}, {}, {});
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

#undef handle_unexpected
    } while(1);

    return Package<ctx::context_t>(std::move(cur_ctx));
    // return Package<ctx::context_t>::makeBroken("Failed task.");
}

Package<ctx::context_t>
parseModule(void) noexcept {
    return Package<ctx::context_t>::makeBroken("Failed to parse module.");
}

NSRCXEND
