#ifndef RCX_1_CTX_CONTEXT_HPP
#define RCX_1_CTX_CONTEXT_HPP

#include <initializer_list>
#include <utility>
#include <variant>

#include <conv/Modernizer.hpp>

#include <llvm/IR/Module.h>

// check if the class is involved in `context_t`
#define CHK_CLS(clsName) \
using __chK_t = std::enable_if_t<does_variant_have_v<clsName, context_t>>

NSRCXBGN
namespace ctx {

template<class> class Context;
class ModuleContext;
class FunctionContext;

// NOTE: should be modified if context is added
using context_t = typename std::variant<ModuleContext, FunctionContext>;

template <typename T>
class BaseContext {
public:
    // BaseContext() = delete;
    ~BaseContext() = default;
};

// global definitions, declarations, types
class ModuleContext : public BaseContext<ModuleContext> {
public:
    using defs_t = typename std::variant<ModuleContext, FunctionContext>;

private:
    std::vector<defs_t> ctx_;

    static
    llvm::Module mod_;

public:

    CHK_CLS(ModuleContext);

    ModuleContext() = delete;
    ~ModuleContext() = default;

    ModuleContext(std::initializer_list<defs_t> && il)
    : ctx_(std::move(il)) {}
};

class FunctionContext : public BaseContext<FunctionContext> {
public:
    CHK_CLS(FunctionContext);

    FunctionContext() = delete;
    ~FunctionContext() = default;

private:
    
};

} // ns ctx
NSRCXEND

#endif