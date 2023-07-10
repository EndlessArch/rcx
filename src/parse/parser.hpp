#ifndef RCX_PARSE_PARSER_HPP
#define RCX_PARSE_PARSER_HPP

#include <parse/AST/AST.hpp>
#include <parse/CTX/Context.hpp>
#include <conv/Modernizer.hpp>

#include <boost/program_options/variables_map.hpp>

#include <llvm/ADT/StringMap.h>

NSRCXBGN

namespace parser {

enum class Token {

// Comment // #
Digit,

Namespace, // namespace
Identifier,
Parentheses, // ()
Braces, // {}
Brackets, // []
Angles, // <>

Comma, // ,

Type, // :
TypeArrow, // ->
TypeDyn, // ?
Annotation, // @

Or, // |
And, // &

KeyCase, // case
KeyReturn, // ret
KeyThen, // =>
KeyStruct, // struct

};

Token
tokenizeIdf(std::string&) noexcept;

std::string
stringifyTok(Token) noexcept;

using expr_t =
  decltype(merge_variant_t(
    fill_every_case<
      ast::BOp,
      ast::Function,
      ast::Call /* INSERT */>(),
    std::declval<std::variant<ast::Call>>()
  ));
using metavars_t = std::vector<std::pair<std::string, expr_t>>;

template <typename F>
Package<metavars_t>
parseMetaVars(F&) noexcept;

} // ns parser

Package<ctx::context_t>
parseStart(llvm::StringMap<boost::program_options::variable_value> &&) noexcept;

// craft module
Package<ctx::context_t>
parseModule(void) noexcept;

NSRCXEND

#endif