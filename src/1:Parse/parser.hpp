#ifndef RCX_1_PARSER_HPP
#define RCX_1_PARSER_HPP

#include <1:Parse/CTX/Context.hpp>
#include <conv/Modernizer.hpp>

#include <boost/program_options/variables_map.hpp>

#include <llvm/ADT/StringMap.h>

NSRCXBGN

Package<ctx::context_t>
parseStart(llvm::StringMap<boost::program_options::variable_value> &&) noexcept;

enum class Keyword {
    Unexpected,
    ExportModule,
    ImportModule,
    Defun,
    DefVar
};

Package<Keyword> parseKeyword(std::string &) noexcept;

Package<ctx::context_t> parseModule(void) noexcept;

NSRCXEND

#endif