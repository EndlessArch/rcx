#ifndef RCX_PARSE_PARSER_HPP
#define RCX_PARSE_PARSER_HPP

#include <parse/CTX/Context.hpp>
#include <conv/Modernizer.hpp>

#include <boost/program_options/variables_map.hpp>

#include <llvm/ADT/StringMap.h>

NSRCXBGN

namespace parser {

enum class Token {

};

Package<Token>
tokenizeIdf() noexcept;

} // ns parser

Package<ctx::context_t>
parseStart(llvm::StringMap<boost::program_options::variable_value> &&) noexcept;

// craft module
Package<ctx::context_t>
parseModule(void) noexcept;

NSRCXEND

#endif