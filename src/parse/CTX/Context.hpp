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
class SpaceContext;

template<typename,typename> class PropertyContext;
class ClassContext;
class FunctionContext;

// NOTE: should be modified if context is added
using context_t = typename std::variant<SpaceContext, ClassContext, FunctionContext>;

template <typename T, typename D>
class BaseContext {
    D& parent_;
public:
    BaseContext(D& parent) : parent_(parent) {}
    ~BaseContext() = default;

    BaseContext&
    operator=(const BaseContext& rhs) {
        parent_ = rhs.parent_;
        return *this;
    }
};

// namespace of definitions
class SpaceContext : public BaseContext<SpaceContext, context_t> {
public:
    using defs_t = context_t;

private:
    std::string name_;
    std::vector<defs_t> ctx_;

    static
    llvm::Module mod_;

public:

    CHK_CLS(SpaceContext);

    SpaceContext() = delete;
    ~SpaceContext() = default;

    SpaceContext(const std::string & name, std::initializer_list<defs_t> && il)
    : BaseContext(*this), name_(name), ctx_(std::move(il)) {}

    inline
    defs_t& addDef(const context_t& rhs) noexcept {
        ctx_.push_back(rhs);
        return ctx_.back();
    }
};

// has annotation
template <typename T, typename U>
class PropertyContext : public BaseContext<PropertyContext<T, U>, U> {
public:
    // CHK_CLS(PropertyContext);

    PropertyContext() = delete;
    ~PropertyContext() = default;

    PropertyContext(std::vector<std::string>&& il)
    : BaseContext<PropertyContext<T, U>, U>(*this), anno_(std::move(il)) {}

    inline
    PropertyContext&
    addAnno(const std::string& rhs) noexcept {
        anno_.push_back(rhs);
    }

private:
    std::vector<std::string> anno_;
};

class ClassContext : public PropertyContext<ClassContext, context_t> {
public:
    CHK_CLS(ClassContext);

    ClassContext() = delete;
    ~ClassContext() = default;
};

class FunctionContext : public PropertyContext<FunctionContext, context_t> {
public:
    CHK_CLS(FunctionContext);

    FunctionContext() = delete;
    ~FunctionContext() = default;

    FunctionContext(std::vector<std::string>&& il)
    : PropertyContext(std::move(il)) {}
};

} // ns ctx
NSRCXEND

#endif