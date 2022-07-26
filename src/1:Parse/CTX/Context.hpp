#ifndef RCX_1_CTX_CONTEXT_HPP
#define RCX_1_CTX_CONTEXT_HPP

#include <utility>
#include <variant>

#include <conv/Modernizer.hpp>

#define ACTIVATE_CLASS(clsName) \
using __chK_t = std::enable_if_t<does_variant_have_v<clsName, context_t>>

NSRCXBGN
namespace ctx {

template<class> class Context;
class ModuleContext;

using context_t = typename std::variant<ModuleContext>;

template <typename T>
class BaseContext {
public:
    BaseContext() = delete;
    ~BaseContext() = default;
};

class ModuleContext : public BaseContext<ModuleContext> {
public:
    ACTIVATE_CLASS(ModuleContext);

    ModuleContext() = delete;
    ~ModuleContext() = default;
    
};

} // ns ctx
NSRCXEND

#endif