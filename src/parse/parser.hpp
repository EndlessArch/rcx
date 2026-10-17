#ifndef RCX_PARSE_PARSER_HPP
#define RCX_PARSE_PARSER_HPP

#include <fstream>

#include <conv/Variantical.hpp>

#include <parse/AST/AST.hpp>
#include <parse/CTX/Context.hpp>

// #include <argparse.h>

// forward declaration
namespace argparse {
  class ArgumentParser;
} // ns argparse

NSRCXBGN

namespace parser {

enum class EToken {

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
Not, // !
Or, // |
And, // &

KeyCase, // case
KeyReturn, // ret
KeyThen, // =>
KeyStruct, // struct

};

EToken
tokenizeIdf(std::string_view) noexcept;

std::string_view
stringifyTok(EToken) noexcept;

struct Token {
  EToken kind;
  std::string lexeme;

  ;
};

struct TokenErrC {
  Token token;
  std::string_view error;

  ;
};

using expr_t =
  decltype(variant::merge_variant_t(
    std::declval<
      variant::fill_every_case<
        ast::BOp,
        ast::Function,
        ast::Call /* INSERT */>>(),
    std::declval<std::variant<ast::Call>>()
  ));
using metavars_t = std::vector<std::pair<std::string, expr_t>>;

template <typename F>
Package<metavars_t>
parseMetaVars(F&) noexcept;

class SourceLexer {
  std::fstream& f_src_;
  std::streampos f_last_pos_;
  std::string buf_{};
  unsigned len_{};

  static constexpr char c_comment = '#';

public:
  explicit SourceLexer(std::fstream& f_src) noexcept
  : f_src_(f_src)
  , f_last_pos_{f_src.tellg()} {}

  Package<Token, EToken> operator()(const auto& expected) noexcept;
  Token operator()() noexcept;

private:

  std::optional<std::string>
  parseStr(auto& it, auto& str, char chH, char chN) noexcept {
    if(str.front() == chH) {
      if(str.length()> 1 && str[1] == chN) {
        auto a = std::string(it, it+ 1);
        str = str.substr(2);
        return a;
      }
    }

    return {};
  }

  template <std::size_t N>
  std::optional<std::string>
  parseVStr(auto& it, auto& str, char chH, std::array<char, N> vch) noexcept {
    if(str.front() != chH)  return {};
    if(str.length() <= 1) return {};

    for(auto c : vch) {
      if(c == str[1]) {
        auto a = std::string(it, it+ 1);
        str = str.substr(2);
        return a;
      }
    }

    return {};
  }

  inline auto getPos() const noexcept {
    return f_last_pos_;
  }

  inline void setPos() noexcept {
    f_last_pos_ = f_src_.tellg();
  }
};

} // ns parser

Package<rcx::ctx::SpaceContext>
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
Package<ctx::SpaceContext>
parseModule(void) noexcept;

NSRCXEND

#include <parse/parser.tpp>

#endif
