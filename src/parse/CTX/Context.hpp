#ifndef RCX_CTX_CONTEXT_HPP
#define RCX_CTX_CONTEXT_HPP

#include "ContextBase.hpp"
#include <initializer_list>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>

#include <conv/Lazy.hpp>
// #include <conv/Modernizer.hpp>
#include <conv/Variantical.hpp>
#include <parse/CTX/ContextBase.hpp>

// // check if the class is involved in `context_t`
// using namespace rcx::variant;
// #define CHK_CLS(clsName) \
//   using __chK_t = std::enable_if_t<does_variant_have_v<clsName, context_t>>

#include <llvm/IR/Module.h>

NSRCXBGN
namespace ctx {

class SpaceContext;
class ClassContext;
class FunctionContext;

class PrimitiveExpression {
public:
    enum class ExpressionType {
        Variable,
        ValueInt,
        ValueFloat,
        ValueString
    };

private:
    ExpressionType exprType_;
    std::variant<int, float, std::string> value_;

    PrimitiveExpression(ExpressionType exprType, auto&& value)
    {}
};

class ExpressionContext : public BaseContext<ExpressionContext> {
public:
    /*
     * - : Retriveable | Modifyable
     * Define : True | True
     * Set LHS : True | True
     * Set RHS : True | False
     * Return : N/A; Both inappropriate
     */
    enum class ExpressionType {
        DefineVariable, // define without value
        InitializeVariable, // define with value
        SetValue,

        ReturnVariable,
        ReturnValue // on primitive value
    };

private:
    ExpressionType type_;
    std::string varName_;
    std::unique_ptr<ExpressionContext> exprType_;
    std::unique_ptr<ExpressionContext> exprValue_;

    static
    ExpressionContext null() noexcept {
        return ExpressionContext{};
    }

public:
    // ExpressionContext(const ExpressionContext& ec) = delete;
    // ExpressionContext(ExpressionContext&& ec) = delete;
    ~ExpressionContext() = default;

    ExpressionContext()
    : BaseContext<ExpressionContext>{BaseContext::null()} {}

    ExpressionContext(const ExpressionContext& ec)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , type_{ec.type_}
    , varName_{ec.varName_}
    , exprType_{std::make_unique<ExpressionContext>(*ec.exprType_)}
    , exprValue_{std::make_unique<ExpressionContext>(*ec.exprValue_)} {}

    ExpressionContext(ExpressionContext&& ec)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , type_{ec.type_}
    , varName_{ec.varName_}
    , exprType_{std::move(ec.exprType_)}
    , exprValue_{std::move(ec.exprValue_)} {}

    // constructor for define
    ExpressionContext(std::string varName, ExpressionContext&& type)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , type_{ExpressionType::DefineVariable}
    , varName_{varName}
    , exprType_{std::make_unique<ExpressionContext>(std::move(type))}
    , exprValue_{} {}

    ExpressionContext(std::string varName, ExpressionContext&& type, ExpressionContext&& value)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , type_{ExpressionType::InitializeVariable}
    , varName_{varName}
    , exprType_{std::make_unique<ExpressionContext>(std::move(type))}
    , exprValue_{std::make_unique<ExpressionContext>(std::move(value))} {}

public:
    inline constexpr
    bool isResultModifyable() noexcept {
        switch(type_) {
        case ExpressionType::ReturnValue:
        case ExpressionType::ReturnVariable:
            return false;
        default:;
        }
        return true;
    }

    static ExpressionContext
    Set(std::string name, ExpressionContext&& value) noexcept {
        ExpressionContext ec{};
        ec.type_ = ExpressionType::SetValue;
        ec.varName_ = name;
        ec.exprValue_ = std::make_unique<ExpressionContext>(std::move(value));
        return ec;
    }
};

// providing space for class {member variable, methods}, functions
class SpaceContext : public BaseContext<SpaceContext> {
public:
    using defs_t = //typename std::variant<SpaceContext*, ClassContext*, FunctionContext*>; // context_t;
        typename rcx::variant::variant_apply_class_t<std::shared_ptr,
            ExpressionContext, SpaceContext, ClassContext, FunctionContext>;

private:
    std::string name_;
    std::vector<defs_t> ctx_;

    static
    llvm::Module mod_;

    static
    SpaceContext null() noexcept {
        // auto base_nil = BaseContext<SpaceContext>::null();
        return SpaceContext(std::string_view{"(null)"}, {});
    }
public:

    // CHK_CLS(SpaceContext);

    // be copy constructible
    // SpaceContext() = delete;
    // SpaceContext(const SpaceContext&) = default;
    // SpaceContext()
    // : BaseContext<SpaceContext>()
    // , name_("ctx_spc")
    // , ctx_{} {}

    SpaceContext() : BaseContext<SpaceContext>(BaseContext::null()) {}

    SpaceContext(const SpaceContext& sc)
    : BaseContext<SpaceContext>{BaseContext::null()}
    , name_{sc.name_}
    , ctx_{ sc.ctx_
        //   | std::views::transform([](const ctxnode_p& a) { return std::make_unique<struct ContextNode>(*a); })
          | std::views::common
          | rcx::lazy::to_vector } {}

    SpaceContext(SpaceContext&& sc)
    : BaseContext<SpaceContext>{BaseContext::null()}
    , name_{std::move(sc.name_)}
    , ctx_{std::move(sc.ctx_)} {}

    ~SpaceContext() = default;

    template <typename S,
        typename = std::enable_if_t<
            std::conjunction_v<
                std::disjunction<
                    rcx::is_basically<S, std::string>,
                    std::is_convertible<S, std::string>,
                    std::is_constructible<std::string, S> >>> >
    SpaceContext(S&& name, std::initializer_list<defs_t> && il)
    : BaseContext<SpaceContext>{BaseContext::null()}
    , name_{std::forward<S>(name)}
    , ctx_( il // no point of moving on initializer_list since its const
        //   | std::views::transform([](auto&& a) { return std::make_unique<struct ContextNode>(std::move(a)); })
          | std::views::common
          | rcx::lazy::to_vector ) {}

    auto& setName(const std::string& name) noexcept {
        name_ = name;
        return *this;
    }

    // NOTE: since C++17; std::vector::emplace_back method returns reference on return
    // auto& addDef(struct ContextNode&& rhs) noexcept {
    //     auto k = std::make_unique<struct ContextNode>(rhs);
    //     return ctx_.emplace_back(std::move(k));
    // }

    auto& addDef(auto&& rhs) noexcept {
        // return \
        // std::visit([this](auto&& rhs) -> defs_t& {
        //     using rhs_t = std::remove_reference_t<decltype(rhs)>;
        //     return ctx_.emplace_back(std::forward<rhs_t>(rhs));
        // }, std::forward<defs_t>(rhs));

        if constexpr (std::is_pointer_v<decltype(rhs)>) {
            return ctx_.emplace_back(std::forward<decltype(rhs)>(rhs));
        }

        if constexpr (rcx::is_kind_of_v<std::shared_ptr, decltype(rhs)>) {
            return ctx_.emplace_back(std::forward<decltype(rhs)>(rhs));
        } else {
            using rhs_t = std::remove_cvref_t<decltype(rhs)>;
            // return ctx_.emplace_back(std::shared_ptr<std::remove_reference_t<decltype(rhs)>>{std::forward<decltype(rhs)>(rhs)});
            std::shared_ptr<rhs_t> shp = std::make_shared<rhs_t>(std::forward<rhs_t>(rhs));
            defs_t vart = shp;
            // ctx_.push_back(vart);
            // defs_t vart(std::in_place_type<std::shared_ptr<rhs_t>>, std::move(shp));
            // defs_t vart = std::move(shp);
            ctx_.push_back(std::move(vart));
            return ctx_.back();
        }
    }

    inline
    unsigned dumpDef() const noexcept {
        if(ctx_.empty()) return 0;

        unsigned sz = ctx_.size();
        std::vector<std::string> vDef(sz);

        for(auto& v : ctx_) {
            vDef.emplace_back(
                std::visit<std::string>(
                    overloaded {
                        [](const std::shared_ptr<ExpressionContext>&) { return "ExpressionContext"; },
                        [](const std::shared_ptr<SpaceContext>&) { return "SpaceContext"; },
                        [](const std::shared_ptr<ClassContext>&) { return "ClassContext"; },
                        [](const std::shared_ptr<FunctionContext>&) { return "FunctionContext"; }
                    }, v));
            spdlog::debug("dumpDef: {}", vDef.back());
        }

        return sz;
    }
};

// has annotation, has space (= body)
template <typename T>
class PropertyContext : public BaseContext<PropertyContext<T>> {
public:
    // CHK_CLS(PropertyContext);

    // PropertyContext() = delete;
    PropertyContext(const PropertyContext<T>& pc)
    : BaseContext<PropertyContext<T>>{BaseContext<PropertyContext>::null()}
    , anno_{pc.anno_}
    , spc_{pc.spc_} {}

    PropertyContext(PropertyContext<T>&& pc)
    : BaseContext<PropertyContext<T>>{BaseContext<PropertyContext>::null()}
    , anno_{std::move(pc.anno_)}
    , spc_{std::move(pc.spc_)} {}

    ~PropertyContext() = default;

    PropertyContext(std::vector<std::string>&& il)
    :   BaseContext<PropertyContext<T>>{BaseContext<PropertyContext>::null()},
        anno_(std::move(il)),
        spc_(*this, "property_space", {}) {}

    PropertyContext(std::vector<std::string>&& il, SpaceContext&& spc)
    :   BaseContext<PropertyContext<T>>{BaseContext<PropertyContext>::null()},
        anno_(std::move(il)),
        spc_(std::move(spc)) {}

    inline PropertyContext&
    addAnno(const std::string& rhs) noexcept {
        anno_.push_back(rhs);
    }

    inline auto&
    getAnno() noexcept {
        return anno_;
    }

    inline SpaceContext&
    getSpace() noexcept {
        return spc_;
    }

private:
    std::vector<std::string> anno_;
    SpaceContext spc_;
};

class ClassContext : public PropertyContext<ClassContext> {
public:
    // CHK_CLS(ClassContext);

    // ClassContext() = delete;
    ClassContext(const ClassContext& cc)
    : PropertyContext<ClassContext>(cc) {}

    ClassContext(ClassContext&& cc)
    : PropertyContext<ClassContext>(std::move(cc)) {}

    ~ClassContext() = default;
};

class FunctionContext : public PropertyContext<FunctionContext> {
public:
    template <class... Args>
    using array_t = std::vector<Args...>;
    using element_t = std::pair<std::string, std::string>;

    using arg_t = array_t<element_t>;
    using attr_t = array_t<std::string>;

private:
    std::optional<attr_t> attrs_;
    std::string name_;
    // arg_t args_;
    // std::shared_ptr<SpaceContext> body_;

    inline
    attr_t& getAttrs() noexcept {
        return static_cast<PropertyContext<FunctionContext>&>(*this).getAnno();
    }

    inline
    SpaceContext& getBody() noexcept {
        return static_cast<PropertyContext<FunctionContext>&>(*this).getSpace();
    }

public:
    // CHK_CLS(FunctionContext);

    // FunctionContext() = delete;
    FunctionContext(const FunctionContext& fc)
    : PropertyContext<FunctionContext>{fc} {}

    FunctionContext(FunctionContext&& fc)
    : PropertyContext<FunctionContext>(std::move(fc)) {}

    // FunctionContext(parent, anno)
    // : PropertyContext<FunctionContext>{parent, annotation} {}

    FunctionContext(const std::string& name, arg_t&& args,
        SpaceContext&& body, std::optional<attr_t> attrs = {})
        : PropertyContext(std::move(*attrs), std::move(body)) {}

    ~FunctionContext() = default;

    template <typename T>
    FunctionContext(T& parent, std::vector<std::string>&& il)
    : PropertyContext(parent, std::move(il)) {}
};

} // ns ctx
NSRCXEND

#endif
