#include <iostream>

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

auto rcx_main(
    [[maybe_unused]] int argc,
    [[maybe_unused]] char * argv[]) -> int {
    ;

    llvm::StringMap<boost::program_options::variable_value> optMap;

    try {
        using namespace boost::program_options;
        options_description desc("Options");

        desc.add_options()
        ("help,h", "Show this")
        // (",s", value<std::string>()->value_name("input_file"), "Specify target source file to compile")
        (",o", value<std::string>()->value_name("output_name"), "Set executable name")
        ("source,s", value<std::string>()->value_name("input_file"), "source");

        positional_options_description pos_desc;
        pos_desc.add("source", -1);
        
        command_line_parser clp(argc, argv);

        clp.options(desc).positional(pos_desc);

        variables_map vm;
        store(clp.run(), vm);
        notify(vm);

        if (vm.count("help") || vm.count("h") || vm.empty() || !vm.count("source")) {
[[maybe_unused]] LABEL_HELP:
            std::cerr << "Usage: rcx [-o output_file] [options...] source_file\n";
            std::cerr << desc;
            return 0;
        }

        for(auto it = vm.begin(); it != vm.end(); ++it)
        optMap[it->first] = it->second;
    } catch(const boost::program_options::error & e) {
        spdlog::error(e.what());
    }

    auto ctx = rcx::parseStart(std::move(optMap))();

    std::get<rcx::ctx::ModuleContext>(ctx);

    return 0;
}