#include <exception>
#include <iostream>
#include <memory>

#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>

#include <argparse.h>
#include <spdlog/spdlog.h>

#include <parse/parser.hpp>
#include <conv/Modernizer.hpp>

auto main(
    /*[[maybe_unused]]*/int argc,
    /*[[maybe_unused]]*/char * argv[]) -> int {
    ; // editor's auto-indentation is stupid

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    argparse::ArgumentParser program("rcx", "1.0.0");
    program.enable_help();

    program.add_argument("-s", "--source",
        "an input code file path", true);
    
    program.add_argument("-o",
        "path for executable output result", false);

    argparse::ArgumentParser::Result res;
    try {
        res = program.parse(argc, (const char **)argv);
    } catch(const std::runtime_error& e) {
        std::cerr << e.what() << '\n';
        std::cerr << res;
        std::abort();
    }

    auto ctx_ = rcx::parseStart(std::move(program))();
    if(ctx_.has_value())
        spdlog::info("Package receive successful");
    else
        spdlog::info("Got nothing");

    return 0;
}
