#ifndef RCX_CTX_CONTEXT_HPP
#define RCX_CTX_CONTEXT_HPP

#include "ContextBase.hpp"
#include <initializer_list>
#include <iterator>
#include <memory>
#include <numeric>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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

class LayoutValueExpression;
class LayoutEntityClass;
class LayoutEntityFunction;

namespace ctx {

class LocalSpaceContext;
class SpaceContext;
class ClassContext;
class FunctionContext;

class ExpressionObject {
public:
    enum class ExpressionObjectType {
        Void,
        EntityName,
        ValueInt,
        ValueFloat,
        ValueString
    };

    struct ScopedName {
    public:
    private:
        std::vector<std::string> scopes_;
    public:
        ScopedName(std::string name)
        : scopes_{name} {}

        ScopedName(std::vector<std::string>&& scopes)
        : scopes_{std::move(scopes)} {}

        inline bool isEmpty() const noexcept {
            return scopes_.empty();
        }

        inline bool isScoped() const noexcept {
            return scopes_.size() > 1;
        }

        std::string name() const noexcept {
            return scopes_.back();
        }

        std::string fullname() const noexcept {
            if(scopes_.empty()) { ; }
            return std::accumulate(
                std::next(scopes_.begin()), scopes_.end(), scopes_[0],
                [](std::string a, const std::string& b) { return a + "::" + b; });
        }
    };

    using prim_value_t = std::variant<int, float, std::string>;

private:
    ExpressionObjectType exprType_;
    std::variant<ScopedName, prim_value_t> value_;

public:
    ExpressionObject()
    : exprType_{ExpressionObjectType::Void}
    , value_{0} {}

    ExpressionObject(const ExpressionObject& eo)
    : exprType_{eo.exprType_}
    , value_{eo.value_} {}

    ExpressionObject(ExpressionObject&& eo)
    : exprType_{eo.exprType_}
    , value_{std::move(eo.value_)} {}

    ExpressionObject(std::string name)
    : exprType_{ExpressionObjectType::EntityName}
    , value_{ScopedName{std::move(name)}} {}

    ExpressionObject(std::vector<std::string>&& scope)
    : exprType_{ExpressionObjectType::EntityName}
    , value_{ScopedName{std::move(scope)}} {}

    ExpressionObject(ExpressionObjectType exprType, auto&& value)
    : exprType_{exprType}
    , value_{prim_value_t(std::forward<decltype(value)>(value))} {}

    inline bool isVoid() const noexcept {
        return exprType_ == ExpressionObjectType::Void;
    }

    inline bool isEmpty() const noexcept {
        return isVoid() &&
            std::visit(overloaded{
                [](const ScopedName& a) { return a.isEmpty(); },
                [](auto&&) { return true; }
            }
        , value_);
    }

    inline
    std::string toString() const noexcept {
        return std::visit(
            overloaded{
                [](const ScopedName& a) noexcept -> std::string { return a.fullname(); },
                [](auto&&) noexcept -> std::string { return ""; }
            }
        , value_);
    }
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
        Value,
        Return,

        DefineVariable, // define without value
        InitializeVariable, // define with value
        SetValue,

        Refer,
        CallFunction
    };

    struct ExpressionEntityBase {
        const ExpressionType type_;

        ExpressionEntityBase(const ExpressionEntityBase& eb)
        : type_{eb.type_} {}

        constexpr ExpressionEntityBase(ExpressionType type)
        : type_{type} {}
    };

    // Value / Return
    struct ExpressionEntityDynamic : ExpressionEntityBase {
        std::unique_ptr<ExpressionContext> expr_;

        ExpressionEntityDynamic(const ExpressionEntityDynamic& eDyn)
        : ExpressionEntityBase(eDyn)
        , expr_{std::make_unique<ExpressionContext>(*eDyn.expr_)} {}

        ExpressionEntityDynamic(ExpressionType type, ExpressionContext ec)
        : ExpressionEntityBase{type}
        , expr_{std::make_unique<ExpressionContext>(std::move(ec))} {}

        // TODO:
        inline
        bool isResultModifyable() const noexcept {
            return expr_->isResultModifyable();
        }

        std::string toString() const noexcept {
            switch(type_) {
            case ExpressionType::Return:
                return "[Return]: " + expr_->toString();
            case ExpressionType::Value:
                return "[Value]: " + expr_->toString();
            default:
                return "(ExpressionEntityDynamic)";
            }
        }
    };

    struct ExpressionOf : ExpressionEntityDynamic {
        ExpressionObject obj_;

        ExpressionOf(const ExpressionOf& eOf)
        : ExpressionEntityDynamic{eOf}
        , obj_{eOf.obj_} {}

        ExpressionOf(ExpressionObject obj, ExpressionContext ec)
        : ExpressionEntityDynamic{ExpressionType::Value, std::move(ec)}
        , obj_{std::move(obj)} {}

        inline
        bool isResultModifyable() const noexcept {
            return expr_->isResultModifyable();
        }

        std::string toString() const noexcept {
            return "[Of]: " + obj_.toString() + "." + expr_->toString();
        }
    };

    struct ExpressionOpNegate : ExpressionEntityDynamic {
        ExpressionOpNegate(const ExpressionOpNegate& op)
        : ExpressionEntityDynamic{op} {}

        ExpressionOpNegate(ExpressionContext ec)
        : ExpressionEntityDynamic{ExpressionType::Value, std::move(ec)} {}

        inline
        bool isResultModifyable() const noexcept {
            return expr_->isResultModifyable();
        }

        std::string toString() const noexcept {
            return "[OpNegate]: " + expr_->toString();
        }
    };

    struct ExpressionEntityReturnVoid : ExpressionEntityBase {
        ExpressionEntityReturnVoid(const ExpressionEntityReturnVoid& e)
        : ExpressionEntityBase{ExpressionType::Return} {}

        constexpr ExpressionEntityReturnVoid()
        : ExpressionEntityBase{ExpressionType::Return} {}
    };

    // Refer
    struct ExpressionEntity : ExpressionEntityBase {
        ExpressionObject obj_;

        ExpressionEntity(const ExpressionEntity& ent)
        : ExpressionEntityBase{ent}
        , obj_{ent.obj_} {}

        ExpressionEntity(ExpressionType type, ExpressionObject&& eo)
        : ExpressionEntityBase{type}
        , obj_{std::move(eo)} {}

        inline
        bool isReferring() const noexcept {
            return type_ == ExpressionType::Refer;
        }

        std::string toString() const noexcept {
            switch(type_) {
            case ExpressionType::Refer:
                return "[Refer]: " + obj_.toString();
            default:
                return "(ExpressionEntity)";
            }
        }
    };

    struct ExpressionEntityDefine : ExpressionEntity {
        using expression_type_t = std::variant<ExpressionObject, ExpressionContext>;

        std::unique_ptr<expression_type_t> eType_;
        std::unique_ptr<ExpressionContext> value_;

        ExpressionEntityDefine(const ExpressionEntityDefine& eed)
        : ExpressionEntity{eed}
        , eType_{std::make_unique<expression_type_t>(*eed.eType_)}
        , value_{std::make_unique<ExpressionContext>(*eed.value_)} {}
            
        ExpressionEntityDefine(ExpressionObject eo, ExpressionContext eType)
        : ExpressionEntity{ExpressionType::DefineVariable, std::move(eo)}
        , eType_{std::make_unique<expression_type_t>(std::move(eType))}
        , value_{} {}

        ExpressionEntityDefine(ExpressionObject eo, ExpressionContext eType, auto&& value)
        : ExpressionEntity{ExpressionType::InitializeVariable, std::move(eo)}
        , eType_{std::make_unique<expression_type_t>(std::move(eType))}
        , value_{std::make_unique<ExpressionContext>(std::forward<decltype(value)>(value))} {}

        std::string toString() const noexcept {
            auto sObj = obj_.toString();
            auto sType = std::visit(
                [](auto&& e) noexcept -> std::string {
                    return e.toString();
                }
            , *eType_);

            if(type_ == ExpressionType::InitializeVariable)
                return "[InitVariable]: " + sObj + ": <" + sType + "> := " + value_->toString();
            
            return "[DefineVariable]: " + sObj + ": <" + sType + ">";
        }
    };

    struct ExpressionEntitySet : ExpressionEntity {
        std::unique_ptr<ExpressionContext> value_;

        ExpressionEntitySet(const ExpressionEntitySet& ees)
        : ExpressionEntity{ees}
        , value_{std::make_unique<ExpressionContext>(*ees.value_)} {}

        ExpressionEntitySet(ExpressionObject eo, ExpressionContext value)
        : ExpressionEntity{ExpressionType::SetValue, std::move(eo)}
        , value_{std::make_unique<ExpressionContext>(std::move(value))} {}

        inline
        std::string toString() const noexcept {
            return "[Set]: " + obj_.toString() + " = " + value_->toString();
        }
    };

    struct ExpressionEntityCall : ExpressionEntity {
        using context_t = ExpressionContext; // std::unique_ptr<ExpressionContext>;
        std::vector<context_t> args_;

        ExpressionEntityCall(const ExpressionEntityCall& eec) //= default;
        : ExpressionEntity{eec}
        , args_{eec.args_} {}

        // void
        ExpressionEntityCall(ExpressionObject eo)
        : ExpressionEntity{ExpressionType::CallFunction, std::move(eo)}
        , args_{} {}

        // ExpressionEntityCall(std::vector<unique_context_t>&& args)
        // : args_{std::move(args)} {}

        ExpressionEntityCall(ExpressionObject eo, std::vector<context_t> args)
        : ExpressionEntity{ExpressionType::CallFunction, std::move(eo)}
        , args_{std::move(args)} {}

        inline
        bool isResultModifyable() const noexcept {
            ; // depends on parsing progress
        }

        std::string toString() const noexcept {
            return "[Call]: fn " + obj_.toString();
        }
    };

    struct ExpressionStatic {
        using expression_t = std::variant<ExpressionEntity, ExpressionEntityReturnVoid,
            ExpressionEntityDefine, ExpressionEntitySet, ExpressionEntityCall>;
        expression_t expr_;

        ExpressionStatic(const ExpressionStatic& st)
        : expr_(st.expr_) {}

        ExpressionStatic(ExpressionEntity e)
        : expr_{std::move(e)} {}

        ExpressionStatic(ExpressionEntityReturnVoid e)
        : expr_{std::move(e)} {}

        ExpressionStatic(ExpressionEntityDefine e)
        : expr_{std::move(e)} {}

        ExpressionStatic(ExpressionEntitySet e)
        : expr_{std::move(e)} {}

        ExpressionStatic(ExpressionEntityCall e)
        : expr_{std::move(e)} {}

        constexpr
        bool isResultModifyable() const noexcept {
            return std::visit(overloaded {
                [](ExpressionEntity e) -> bool {
                    return e.isReferring();
                },
                [](ExpressionEntityReturnVoid) -> bool {
                    return false; // void is definitely unmodifyable
                },
                [](ExpressionEntityDefine d) -> bool {
                    return false;
                },
                [](ExpressionEntitySet s) -> bool {
                    return true;
                },
                [](ExpressionEntityCall c) -> bool {
                    return c.isResultModifyable();
                }
            }, expr_);
        }

        inline
        auto getType() const noexcept {
            return std::visit(
                overloaded {
                    [](const ExpressionEntity& e) { return e.type_; },
                    [](const ExpressionEntityReturnVoid&) { return ExpressionType::Return; },
                    [](const ExpressionEntityDefine& d) { return d.type_; },
                    [](const ExpressionEntitySet&) { return ExpressionType::SetValue; },
                    [](const ExpressionEntityCall&) { return ExpressionType::CallFunction; }
                }
            , expr_);
        }

        std::string toString() const noexcept {
            return std::visit(
                overloaded {
                    [](const ExpressionEntity& e) noexcept -> std::string {
                        return e.toString();
                    },
                    [](const ExpressionEntityReturnVoid&) noexcept -> std::string {
                        return "[Return]: (void)";
                    },
                    [](const ExpressionEntityDefine& d) noexcept -> std::string {
                        return d.toString();
                    },
                    [](const ExpressionEntitySet& s) noexcept -> std::string {
                        return s.toString();
                    },
                    [](const ExpressionEntityCall& c) noexcept -> std::string {
                        return c.toString();
                    }
                }
            , expr_);
        }
    };

    struct ExpressionDynamic {
        std::variant<ExpressionEntityDynamic, ExpressionOf, ExpressionOpNegate> dExpr_;

        ExpressionDynamic(const ExpressionDynamic& dyn)
        : dExpr_{dyn.dExpr_} {}

        ExpressionDynamic(ExpressionEntityDynamic dyn)
        : dExpr_{std::move(dyn)} {}

        ExpressionDynamic(ExpressionOf of)
        : dExpr_{std::move(of)} {}

        ExpressionDynamic(ExpressionOpNegate neg)
        : dExpr_{std::move(neg)} {}

        inline constexpr
        bool isReferring() const noexcept {
            return std::visit(
                [](auto&& e) {
                    return e.isResultModifyable();
                }
            , dExpr_);
        }

        inline
        auto getType() const noexcept {
            return std::visit(
                [](auto&& e) {
                    return e.type_;
                }
            , dExpr_);
        }

        inline
        std::string toString() const noexcept {
            return std::visit(
                [](auto&& e) {
                    return e.toString();
                }
            , dExpr_);
        }
    };

private:
    std::variant<ExpressionStatic, ExpressionDynamic> entity_;

    // static
    // ExpressionContext null() noexcept {
    //     return ExpressionContext{};
    // }

public:
    // ExpressionContext(const ExpressionContext& ec) = delete;
    // ExpressionContext(ExpressionContext&& ec) = delete;
    ~ExpressionContext() = default;

    // ExpressionContext()
    // : BaseContext<ExpressionContext>{BaseContext::null()}
    // , entity_{}
    // , comp_{} {}

    ExpressionContext(const ExpressionContext& ec)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{ec.entity_}
    {}

    ExpressionContext(ExpressionContext&& ec)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{std::move(ec.entity_)}
    {}

    // dynamic
    ExpressionContext(ExpressionType type, ExpressionContext expr)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{ExpressionDynamic(ExpressionEntityDynamic(type, std::move(expr)))}
    {}

    ExpressionContext(ExpressionOf e)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{ExpressionDynamic(std::move(e))} {}    

    ExpressionContext(ExpressionOpNegate e)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{ExpressionDynamic(std::move(e))} {}    

    ExpressionContext(ExpressionEntity e)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{ExpressionStatic(std::move(e))} {}

    ExpressionContext(ExpressionEntityReturnVoid e)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{ExpressionStatic(std::move(e))} {}

    ExpressionContext(ExpressionEntityDefine e)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{ExpressionStatic(std::move(e))} {}

    ExpressionContext(ExpressionEntitySet e)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{ExpressionStatic(std::move(e))} {}

    ExpressionContext(ExpressionEntityCall e)
    : BaseContext<ExpressionContext>{BaseContext::null()}
    , entity_{ExpressionStatic(std::move(e))} {}

public:
    inline constexpr
    bool isResultModifyable() const noexcept {
        return std::visit(
            overloaded {
                [](const ExpressionStatic& s) noexcept -> bool {
                    // ExpressionEntitySet
                    return s.isResultModifyable();
                },
                [](const ExpressionDynamic& d) noexcept -> bool {
                    // RefVariable
                    return d.isReferring();
                }
            }
        , entity_);
    }

    inline constexpr
    bool isReferring() const noexcept {
        return false; // TODO
    }

    inline constexpr
    bool isDynamic() const noexcept {
        return std::visit(
            overloaded {
                [](const ExpressionStatic&) noexcept -> bool {
                    return false;
                },
                [](const ExpressionDynamic&) noexcept -> bool {
                    return true;
                }
            }
        , entity_);
    }

    inline
    auto getEntityType() const noexcept {
        return std::visit(
            [](auto&& e) {
                return std::forward<decltype(e)>(e).getType();
            }
        , entity_);
    }

    std::string toString() const noexcept {
        return std::visit(
            [](auto&& e) {
                return e.toString();
            }, entity_);
    }
};

struct ExpressionContextBuilder {
    static
    ExpressionContext buildReturn() noexcept {
        constexpr ExpressionContext::ExpressionEntityReturnVoid e{};

        return ExpressionContext{ExpressionContext{std::move(e)}};
    }

    static
    ExpressionContext buildReturn(ExpressionContext ec) noexcept {
        constexpr auto type = ExpressionContext::ExpressionType::Return;

        return ExpressionContext{type, std::move(ec)};
    }
    
    static
    ExpressionContext buildRef(ExpressionObject obj) noexcept {
        constexpr auto type = ExpressionContext::ExpressionType::Refer;

        ExpressionContext::ExpressionEntity e(type, std::move(obj));
        return ExpressionContext{std::move(e)};
    }

    static
    ExpressionContext buildSet(ExpressionObject obj, ExpressionContext ec) noexcept {
        ExpressionContext::ExpressionEntitySet s{std::move(obj), std::move(ec)};
        return ExpressionContext{std::move(s)};
    }

    static
    ExpressionContext buildCall(ExpressionObject fn, std::vector<ExpressionContext> args) noexcept {
        ExpressionContext::ExpressionEntityCall c{std::move(fn), std::move(args)};
        return ExpressionContext{std::move(c)};
    }

    static
    ExpressionContext buildDefine(ExpressionObject obj, ExpressionContext ec) noexcept {
        ExpressionContext::ExpressionEntityDefine d{std::move(obj), std::move(ec)};
        return ExpressionContext{std::move(d)};
    }

    static
    ExpressionContext buildDefine(ExpressionObject obj, ExpressionContext ec, auto&& value) noexcept {
        ExpressionContext::ExpressionEntityDefine d{std::move(obj), std::move(ec), std::forward<decltype(value)>(value)};
        return ExpressionContext{std::move(d)};
    }

    static
    ExpressionContext buildOf(ExpressionObject obj, ExpressionContext ec) noexcept {
        ExpressionContext::ExpressionOf f{std::move(obj), std::move(ec)};
        return ExpressionContext{std::move(f)};
    }

    static
    ExpressionContext buildOpNegate(ExpressionContext ec) noexcept {
        ExpressionContext::ExpressionOpNegate oNeg{std::move(ec)};
        return ExpressionContext{std::move(oNeg)};
    }
};

// providing space for class {member variable, methods}, functions
class SpaceContext : public BaseContext<SpaceContext> {
public:
    using defs_t = //typename std::variant<SpaceContext*, ClassContext*, FunctionContext*>; // context_t;
        typename rcx::variant::variant_apply_class_t<std::shared_ptr
            , LayoutEntityClass
            , LayoutEntityFunction
            , LocalSpaceContext
            , SpaceContext>;

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

    unsigned dumpDef() const noexcept;
};

class LocalSpaceContext : public BaseContext<LocalSpaceContext> {
    std::string name_;
    std::vector<std::variant<
        std::shared_ptr<LayoutValueExpression>,
        std::shared_ptr<LocalSpaceContext>>> ctx_;

    static
    llvm::Module mod_;

    static
    SpaceContext null() noexcept {
        // auto base_nil = BaseContext<SpaceContext>::null();
        return SpaceContext(std::string_view{"(null)"}, {});
    }
public:
    LocalSpaceContext() : BaseContext<LocalSpaceContext>(BaseContext::null()) {}

    auto& addDef(auto&& rhs) noexcept {
        if constexpr (std::is_pointer_v<decltype(rhs)>) {
            return ctx_.emplace_back(std::forward<decltype(rhs)>(rhs));
        }

        if constexpr (rcx::is_kind_of_v<std::shared_ptr, decltype(rhs)>) {
            return ctx_.emplace_back(std::forward<decltype(rhs)>(rhs));
        } else {
            using rhs_t = std::remove_cvref_t<decltype(rhs)>;
            std::shared_ptr<rhs_t> shp = std::make_shared<rhs_t>(std::forward<rhs_t>(rhs));
            ctx_.emplace_back(std::move(shp));
            return ctx_.back();
        }
    }

    unsigned dumpDef() const noexcept;
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
    using element_t = std::pair<ExpressionObject, ExpressionContext>;

    using arg_t = array_t<element_t>;
    using attr_t = array_t<std::string>;

private:
    std::optional<attr_t> attrs_;
    std::string name_;
    ExpressionObject rType_;
    arg_t args_;
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

    FunctionContext(const std::string& name,
        ExpressionObject ret_type, arg_t&& args,
        SpaceContext&& body, std::optional<attr_t> attrs = {})
        : PropertyContext(std::move(*attrs), std::move(body))
        , rType_{std::move(ret_type)} {}

    ~FunctionContext() = default;

    template <typename T>
    FunctionContext(T& parent, std::vector<std::string>&& il)
    : PropertyContext(parent, std::move(il)) {}

    std::string dumpDef() const noexcept {
        return name_ + "" + rType_.toString();
    }
};

} // ns ctx
NSRCXEND

#endif
