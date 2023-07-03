#include "parser.hpp"
#include "spdlog/fmt/bundled/core.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>

#include <parse/CTX/Context.hpp>
#include <conv/Modernizer.hpp>

#include <boost/program_options/variables_map.hpp>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringExtras.h> // isalnum
#include <llvm/ADT/Triple.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>

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
        { "=>", Token::KeyThen }
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
        { Token::KeyThen, "=>" }
    };
    // spdlog::debug("Token: {}", (int)tok);
    auto a = nameMap.find(tok);
    return a == nameMap.end() ? "identifier" : nameMap.at(tok);
}

} // ns parser

Package<ctx::context_t>
parseStart(llvm::StringMap<boost::program_options::variable_value> && optMap) noexcept {
    ;

    for(auto it = optMap.begin(); it != optMap.end(); ++it)
        spdlog::debug("|MapChk| (" + it->first().str() + ", " + it->second.as<std::string>() + ')');

    std::fstream f_src, f_out;

    {
        auto sourceName = optMap["source"].as<std::string>();
        auto destName = optMap["-o"].empty() ?
            "exec.out" : optMap["-o"].as<std::string>().c_str();

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
            [](char a) {
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
        } else if(buf.front() == '=') {
            if(buf.at(1) == '>') {
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

    auto glob_ctx = ctx::SpaceContext(":)global_ns", {});
    auto& cur_ctx = glob_ctx; // current context
    auto& par_ctx = glob_ctx; // parent context

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

        switch(tok) {
        case Token::Namespace: {
            idf = f_idf();
            tok = tokenizeIdf(idf);
            if(tok != Token::Identifier)
                handle_unexpected(tok, idf);
            ctx::context_t c = ctx::SpaceContext(idf, {});
            glob_ctx.addDef(c);
        }
        case Token::Identifier: {
            ;
        }
        case Token::Annotation: {
            auto a = f_idf();
            auto annos = std::vector<std::string>(2);
            if(a == "{") {
                do {
                    a = f_idf();
                    tok = tokenizeIdf(a);
                    if(tok == Token::Identifier) {
                        annos.push_back(a);
                        a = f_idf();
                        if(a == ",") continue;
                        if(a == "}") break;
                        handle_unexpected(tokenizeIdf(a), a);
                        break; //
                    }
                } while(1);
            } else if((tok = tokenizeIdf(a)) == Token::Identifier)
                annos[0] = a;
            else
                handle_unexpected(tok, a);
            ;
            std::visit(
                [&](auto& arg) {
                    if constexpr(std::is_same_v<decltype(arg), ctx::SpaceContext>) {
                        par_ctx = cur_ctx;
                        cur_ctx = arg;
                    } else
                        return;
                },
                glob_ctx.addDef(ctx::FunctionContext(std::move(annos)))
            );
        }
        case Token::Parentheses: {
            // in case of binary function
            ;
        }
        default:;
            spdlog::warn("Unexpected token: {} ({})", idf, stringifyTok(tok));
        }
#undef handle_unexpected
    } while(1);

    return Package<ctx::context_t>::makeBroken("Failed task.",
        []() noexcept { return ctx::SpaceContext("", {}); });
}

Package<ctx::context_t>
parseModule(void) noexcept {
    return Package<ctx::context_t>::makeBroken("Failed to parse module.",
        []() noexcept { return ctx::SpaceContext("", {}); });
}

NSRCXEND