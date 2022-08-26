#ifndef RCX_PARSE_PARSER_HPP
#define RCX_PARSE_PARSER_HPP

#include <parse/CTX/Context.hpp>
#include <conv/Modernizer.hpp>

NSRCXBGN

namespace parser {

} // ns parser

Package<ctx::context_t>
parseStart(int, char*[]) noexcept;

Package<ctx::context_t> parseModule(void) noexcept;

NSRCXEND

#endif