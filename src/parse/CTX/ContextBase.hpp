#ifndef RCX_CTX_CONTEXTBASE_HPP
#define RCX_CTX_CONTEXTBASE_HPP

#include <memory>

#include <conv/Modernizer.hpp>
#include <parse/CTX/Context.hpp>

NSRCXBGN

namespace ctx {

// template<class,class> class BaseContext;
// class SpaceContext;

// template<class> class PropertyContext;
// class ClassContext;
// class FunctionContext;

// NOTE: should be modified if context is added
// using context_t = typename std::variant<SpaceContext, ClassContext, FunctionContext>;

// using ctx_all_t = 
// using ctxp_t = typename std::variant<>;

// struct ContextNode {
//     std::string ctx_name_;

//     ContextNode(std::string name) : ctx_name_(name) {}
// };
// using ctxnode_p = typename std::unique_ptr<struct ContextNode>;

// class ContextNodeITG {
//     std::vector<ctxnode_p> list_;
// public:

//     ContextNodeITG() : list_{} {}
//     // ContextNodeITG(int k) : list_(k) {}
//     ~ContextNodeITG() {
//         list_.clear();
//     }

//     void clear() {
//         list_.clear();
//     }
// };

// namespace {
// static ContextNode __ctx_none = ContextNode("(null)");
// } // ns anon

template <typename T
    /*, typename = std::enable_if_t<std::is_base_of_v<P, T>>*/ >
class BaseContext {
    // const void* parent_;
    // std::vector<ctxnode_p> childs_;

    std::optional<int> v_;

private:
    BaseContext(void) noexcept = default;
    // : parent_{std::make_unique<struct ContextNode>(__ctx_none)} {}

    inline T& impl() noexcept { return static_cast<T&>(*this); }

public:
    inline constexpr bool isNull() noexcept {
        return v_.has_value();
    }

    constexpr static
    BaseContext<T> null() noexcept {
        return BaseContext<T>();
    }

    BaseContext(const BaseContext<T>& bc) = default;
    // : parent_(std::make_unique<struct ContextNode>(*bc.parent_)) {}
    BaseContext(BaseContext<T>&&) = default;

    // template <typename U,
    //     typename = std::enable_if_t<rcx::is_basically_v<U, struct ContextNode>>>
    // BaseContext(const U& parent) : parent_(std::make_unique<struct ContextNode>(*parent)) {}
    ~BaseContext() {
        // parent_ = nullptr;
        // childs_.clear();
    }

    BaseContext&
    operator=(const BaseContext& rhs) {
        // parent_ = rhs.parent_;
        return *this;
    }

    // auto& childs() noexcept {
    //     return childs_;
    // }

    // ctxnode_p&& nodify() noexcept {
    //     return std::make_unique<struct ContextNode>(*this);
    // }
};

} // ns ctx

NSRCXEND

#endif // RCX_CTX_CONTEXTBASE_HPP