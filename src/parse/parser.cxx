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
        if(cnt) {
            auto a = buf.substr(0, cnt);
            buf = buf.substr(cnt);
            return a;
        }

        cnt = 0;
        while(llvm::isAlnum(*(it + cnt))) ++cnt;
        if(cnt) {
            // alnum identifier
            auto a = buf.substr(0, cnt);
            buf = buf.substr(cnt);
            return a;
        }
        // special character
        auto a = std::to_string(buf.front());
        buf = buf.substr(1);
        // spdlog::debug("a: {}", a.c_str());
        return a;
    };

    do {
        auto idf = f_idf();
        if(idf.empty()) break;
        spdlog::debug("\"Token: {}\"", idf.c_str());
    } while(1);

    return Package<ctx::context_t>::makeBroken("Failed task.",
        []() noexcept { return ctx::ModuleContext({}); });
}

Package<ctx::context_t>
parseModule(void) noexcept {
    return Package<ctx::context_t>::makeBroken("Failed to parse module.",
        []() noexcept { return ctx::ModuleContext({}); });
}

NSRCXEND