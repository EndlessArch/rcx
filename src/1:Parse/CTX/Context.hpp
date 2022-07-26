#ifndef RCX_1_CTX_CONTEXT_HPP
#define RCX_1_CTX_CONTEXT_HPP

#include <utility>
#include <variant>

#include <conv/Modernizer.hpp>

#define CHK_CLS(clsName) \
using __chK_t = std::enable_if_t<does_variant_have_v<clsName, context_t>>

NSRCXBGN
namespace ctx {

template<class> class Context;
class ModuleContext;
class FunctionContext;

using context_t = typename std::variant<ModuleContext, FunctionContext>;

template <typename T>
class BaseContext {
public:
    BaseContext() = delete;
    ~BaseContext() = default;
};

class ModuleContext : public BaseContext<ModuleContext> {
public:
    CHK_CLS(ModuleContext);

    ModuleContext() = delete;
    ~ModuleContext() = default;
    
};

class FunctionContext : public BaseContext<FunctionContext> {
public:
    CHK_CLS(FunctionContext);

    FunctionContext() = delete;
    ~FunctionContext() = default;

    
};

} // ns ctx
NSRCXEND

#endif