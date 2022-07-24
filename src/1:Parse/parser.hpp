#ifndef RCX_1_PARSER_HPP
#define RCX_1_PARSER_HPP

#include <1:Parse/CTX/Context.hpp>
#include <conv/Modernizer.hpp>

#include <boost/program_options/variables_map.hpp>

#include <llvm/ADT/StringMap.h>

NSRCXBGN

Package<ctx::CompileTimeContext>
mainParser(llvm::StringMap<boost::program_options::variable_value> &&) noexcept;

NSRCXEND

#endif