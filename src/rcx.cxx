#include <exception>
#include <iostream>
#include <memory>

// #include <llvm/ADT/StringMap.h>
// #include <llvm/ADT/StringRef.h>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include <parse/parser.hpp>

auto main(
    /*[[maybe_unused]]*/int argc,
    /*[[maybe_unused]]*/char * argv[]) -> int {
    ; // editor's auto-indentation is stupid

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    argparse::ArgumentParser program("rcx", "1.0.0");
    //    program.enable_help();

    program.add_argument("-s", "--source")
      .help("file path to an input code")
      .metavar("SOURCE")
      .required();
    //    program.add_argument("-s", "--source",
    //        "an input code file path", true);

    program.add_argument("-o", "--output")
      .help("directory or path where executable output file installed")
      .metavar("OUTPUT")
      .default_value(std::string("./a.out"));
    //    program.add_argument("-o",
    //        "path for executable output result", false);

    //    argparse::ArgumentParser::Result res;
    try {
        program.parse_args(argc, (const char **)argv);
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        std::cerr << program;
	return -1;
    }

    auto ctx_ = rcx::parseStart(std::move(program))();
    if(ctx_.has_value())
        spdlog::info("Package receive successful");
    else
        spdlog::info("Got nothing");

    return 0;
}
