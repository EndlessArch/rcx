#include "parser.hpp"

#include "1:Parse/CTX/Context.hpp"
#include "conv/Modernizer.hpp"

#include <boost/program_options/variables_map.hpp>

NSRCXBGN

Package<ctx::CompileTimeContext>
mainParser(llvm::StringMap<boost::program_options::variable_value> && optMap) noexcept {
    
    for(auto it = optMap.begin(); it != optMap.end(); ++it)
        spdlog::info(it->first().str());

        
}

NSRCXEND