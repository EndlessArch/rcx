#include <exception>
#include <iostream>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>

#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>

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

    llvm::StringMap<boost::program_options::variable_value> optMap;

    {
        using namespace boost::program_options;
        options_description desc("Options");

        static auto help_exit = \
        [&desc/*= std::move(desc)*/](int exit_code) {
            std::cerr << "Usage: rcx [-o output_file] [options...] source_file\n";
            std::cerr << desc;
            std::exit(exit_code);
        };

        desc.add_options()
        ("help,h", "Show this")
        // an optional source file argument would be parsed from `parse/parser.cxx;parseStart`
        // (",s", value<std::string>()->value_name("input_file"), "Specify target source file to compile")
        (",o", value<std::string>()->value_name("output_name"), "Set executable name")
        ("source,s", value<std::string>()->value_name("input_file")/*->required()*/, "source");

        positional_options_description pos_desc;
        pos_desc.add("source", -1);

        command_line_parser clp(argc, argv);

        clp.options(desc).positional(pos_desc);

        variables_map vm;

        try {
            store(clp.run(), vm);
            notify(vm);
        } catch(const boost::program_options::error & e) {
            spdlog::error(e.what());
            help_exit(-1);
        }

        if (vm.count("help") || vm.count("h") || vm.empty() || !vm.count("source"))
            help_exit(0);

        for(auto it = vm.begin(); it != vm.end(); ++it)
            optMap[it->first] = std::move(it->second);
    }

    auto ctx_ = rcx::parseStart(std::move(optMap));
    auto ctx = std::get<rcx::ctx::SpaceContext>(ctx_());

    spdlog::info("Package opening successful\n");

    return 0;
}
