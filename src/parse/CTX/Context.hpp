#ifndef RCX_CTX_CONTEXT_HPP
#define RCX_CTX_CONTEXT_HPP

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

template<class,class> class BaseContext;
class SpaceContext;

template<class> class PropertyContext;
class ClassContext;
class FunctionContext;

// NOTE: should be modified if context is added
using context_t = typename std::variant<SpaceContext, ClassContext, FunctionContext>;

template <typename T, typename P = SpaceContext>
class BaseContext {
    void* parent_;
    std::vector<context_t> childs_;

    BaseContext(void) noexcept
    : parent_(nullptr), childs_{} {}
public:

    static BaseContext<T, P> null() noexcept {
        return BaseContext<T, P>();
    }

    BaseContext(const BaseContext<T, P>&) = default;

    template <typename U>
    BaseContext(const U& parent) : parent_(&parent), childs_{} {}
    ~BaseContext() = default;

    BaseContext&
    operator=(const BaseContext& rhs) {
        parent_ = rhs.parent_;
        return *this;
    }

    template <typename _T, typename _P>
    inline
    BaseContext<_T, _P>* getParentSpc() noexcept {
        return reinterpret_cast<BaseContext<_T, _P> *>(parent_);
    }

    inline
    auto& childs() noexcept {
        return childs_;
    }
};

// namespace of definitions
class SpaceContext : public BaseContext<SpaceContext> {
public:
    using defs_t = context_t;

private:
    std::string name_;
    std::vector<defs_t> ctx_;

    static
    llvm::Module mod_;

public:

    CHK_CLS(SpaceContext);

    static
    SpaceContext null() noexcept {
        auto null = BaseContext<SpaceContext>::null();
        return SpaceContext(null, "", {});
    }

    SpaceContext() = delete;
    ~SpaceContext() = default;

    template <typename T>
    SpaceContext(T& parent, const std::string & name, std::initializer_list<defs_t> && il)
    : BaseContext<SpaceContext>(parent), name_(name), ctx_(std::move(il)) {}

    inline
    auto& setName(const std::string& name) noexcept {
        name_ = name;
        return *this;
    }

    inline
    defs_t& addDef(const context_t& rhs) noexcept {
        ctx_.push_back(rhs);
        return ctx_.back();
    }
};

// has annotation, has space
template <typename T>
class PropertyContext : public BaseContext<PropertyContext<T>> {
public:
    // CHK_CLS(PropertyContext);

    PropertyContext() = delete;
    ~PropertyContext() = default;

    template <typename P>
    PropertyContext(P& parent, std::vector<std::string>&& il)
    :   BaseContext<PropertyContext<T>>(parent),
        anno_(std::move(il)),
        spc_(*this, "property_space", {}) {}

    inline
    PropertyContext&
    addAnno(const std::string& rhs) noexcept {
        anno_.push_back(rhs);
    }

    inline
    SpaceContext&
    getSpace() noexcept {
        return spc_;
    }

private:
    std::vector<std::string> anno_;
    SpaceContext spc_;
};

class ClassContext : public PropertyContext<ClassContext> {
public:
    CHK_CLS(ClassContext);

    ClassContext() = delete;
    ~ClassContext() = default;
};

class FunctionContext : public PropertyContext<FunctionContext> {
public:
    CHK_CLS(FunctionContext);

    FunctionContext() = delete;
    ~FunctionContext() = default;

    template <typename T>
    FunctionContext(T& parent, std::vector<std::string>&& il)
    : PropertyContext(parent, std::move(il)) {}
};

} // ns ctx
NSRCXEND

#endif