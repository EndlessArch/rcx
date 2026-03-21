#ifndef RCX_PARSE_PARSER_HPP
#define RCX_PARSE_PARSER_HPP

#include <parse/AST/AST.hpp>
#include <parse/CTX/Context.hpp>

// #include <argparse.h>

// forward declaration
namespace argparse {
  class ArgumentParser;  
} // ns argparse

NSRCXBGN

namespace parser {

enum class Token {

__UNKNOWN,
__EOF,

// Comment // #
Digit,
Integer,
Float,

Namespace, // namespace
Identifier,
Parentheses, // ()
Braces, // {}
Brackets, // []
Angles, // <>

VRelations, // >= <= ==
VSet, // =

Comma, // ,
Scope, // ::

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
    std::declval<
      fill_every_case<
        ast::BOp,
        ast::Function,
        ast::Call /* INSERT */>>(),
    std::declval<std::variant<ast::Call>>()
  ));
using metavars_t = std::vector<std::pair<std::string, expr_t>>;

template <typename F>
Package<metavars_t>
parseMetaVars(F&) noexcept;

// template <typename F, template<> class V>
// Package<V<std::string>>
// parseVString(F&) noexcept;

} // ns parser

Package<rcx::ctx::context_t>
parseStart(argparse::ArgumentParser &&) noexcept;

// template <typename T, typename U>
// Package<rcx::ctx::context_t>
// parseFunction(T& readF, U& expectF, ctx::SpaceContext&) noexcept;

// template <typename T, typename U>
// Package<rcx::ctx::context_t>
// parseStruct(T& readF, U& expectF, ctx::SpaceContext&) noexcept;

// template <typename T, typename U>
// Package<rcx::ctx::context_t>
// parseBinaryFunction(T& readF, U& expectF, ctx::SpaceContext&) noexcept;

// template <typename T, typename U>
// Package<rcx::ctx::context_t>
// parseAnnotation(T& readF, U& expectF, ctx::SpaceContext&) noexcept;

// template <typename T, typename U>
// Package<rcx::ctx::context_t>
// parseNamespace(T& readF, U& expectF, ctx::SpaceContext&) noexcept;

// template <typename T, typename U>
// Package<rcx::ctx::context_t>
// parseGlobal(T& readF, U& expectF, ctx::SpaceContext&) noexcept;

// craft module
Package<ctx::context_t>
parseModule(void) noexcept;

NSRCXEND

#include <parse/parser.tpp>

#endif
