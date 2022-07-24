#include <iostream>

#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>

#include <spdlog/spdlog.h>

#include <1:Parse/parser.hpp>
#include <conv/Modernizer.hpp>

auto main(
  [[maybe_unused]] int argc,
  [[maybe_unused]] char * argv[]) -> int {
  ;

  llvm::StringMap<boost::program_options::variable_value> optMap;

  try {
    using namespace boost::program_options;
    options_description desc("Options");

    desc.add_options()
      ("help,h", "Show this")
      ("s", value<std::string>(), "Specify target source file to compile")
      ("o", value<std::string>(), "Set executable name");

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    for(auto it = vm.begin(); it != vm.end(); ++it)
      optMap[it->first] = it->second;
  } catch(const boost::program_options::error & e) {
    spdlog::error(e.what());
  }

  rcx::mainParser(std::move(optMap));

  return 0;
}