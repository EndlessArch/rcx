#include "parser.hpp"

#include <fstream>
#include <functional>
#include <istream>
#include <iterator>

// #include <ranges>

#include <boost/program_options/variables_map.hpp>

#include <llvm/ADT/StringRef.h>

#include <spdlog/spdlog.h>

#include "1:Parse/CTX/Context.hpp"
#include "conv/Modernizer.hpp"

NSRCXBGN

Package<ctx::context_t>
parseStart(llvm::StringMap<boost::program_options::variable_value> && optMap) noexcept {
    
    std::string sourceName = optMap["source"].as<std::string>();
    std::string destName = optMap["-o"].empty() ? "" : optMap["-o"].as<std::string>();

    std::fstream sourceF(sourceName, std::ios_base::in);
    if (!sourceF.is_open()) {
        spdlog::error("Unable to open " + sourceName + ".");
        std::abort();
    }

    static auto parseStreamer = [&sourceF](void) -> std::string {
        ;
        static std::string buf = "";

        if (buf.empty() && std::getline(sourceF, buf).eof())
            return std::string({EOF});
            
        static auto spaceSplitter = [](std::string & rhs) noexcept -> std::string {
            if(auto a = rhs.find(' '); a != std::string::npos) {

                rhs.erase(a);

                return std::string(rhs.begin(), rhs.begin() + a);
            }

            std::string res = std::move(rhs);
            rhs = "";
            return res;
        };

        return spaceSplitter(buf);
    };

    using mod_ctx_t = Package<ctx::context_t>;

    static auto throwNoneWith = [](llvm::StringRef str) -> mod_ctx_t {
        return mod_ctx_t::makeBroken(
        str,
        []() noexcept -> ctx::context_t { 
            return ctx::context_t(ctx::ModuleContext({}));
        });
    };

    auto word = parseStreamer();

    if(word.front() == EOF)
        return throwNoneWith("Faced EOF");

    Keyword ikw = parseKeyword(word)();

    switch (ikw) {
    case Keyword::ExportModule:
        return parseModule();
    default:
        spdlog::error("Expected module only :P");
    }

    return throwNoneWith("unreachable code");
}

Package<Keyword> parseKeyword(std::string & str) noexcept {
    struct __cmpKw {
        auto operator()(const std::string & lhs, const std::string & rhs) const -> bool {
            return rhs.compare(0, 3, lhs);
        }
    };

    static std::map<std::string, Keyword, __cmpKw> __kw_map = {
        { "mod", Keyword::ExportModule },
        { "imp", Keyword::ImportModule },
        { "def", Keyword::Defun }
    };

    Keyword kw = Keyword::Unexpected;

    if(kw = __kw_map[str]; kw != Keyword::Unexpected) {

        str.erase(0, 3);

        return Package<Keyword>(std::move(kw));
    }

    return kw;
}

Package<ctx::context_t>
parseModule(void) noexcept {
}

NSRCXEND