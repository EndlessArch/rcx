#ifndef RCX_PARSE_LAYOUT_LAYOUT_HPP
#define RCX_PARSE_LAYOUT_LAYOUT_HPP

#include <conv/Modernizer.hpp>

#include <numeric>

NSRCXBGN

class LayoutPrimitiveValue {
    std::variant<std::int64_t, double, std::string> value_;
public:
    LayoutPrimitiveValue(const LayoutPrimitiveValue& prim)
    : value_{prim.value_} {}

    LayoutPrimitiveValue(LayoutPrimitiveValue&& prim)
    : value_{std::move(prim.value_)} {}

    LayoutPrimitiveValue(auto&& v)
    : value_{std::forward<decltype(v)>(v)} {}

    ;
};

// Pre-defined (or defining) type-name
class LayoutObjectName {
public:
    struct ObjectName {
    public:
    private:
        std::vector<std::string> scopes_;
    public:
        constexpr ObjectName()
        : scopes_{} {}

        ObjectName(std::string name)
        : scopes_{name} {}

        ObjectName(std::vector<std::string>&& scopes)
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

        std::string toString() const noexcept {
            if(scopes_.empty()) { ; }
            return std::accumulate(
                std::next(scopes_.begin()), scopes_.end(), scopes_[0],
                [](std::string a, const std::string& b) { return a + "::" + b; });
        }
    };

private:
    ObjectName name_;

public:
    constexpr LayoutObjectName()
    : name_{} {}

    constexpr LayoutObjectName(const LayoutObjectName& obj)
    : name_{obj.name_} {}

    constexpr LayoutObjectName(LayoutObjectName&& obj)
    : name_{std::move(obj.name_)} {}

    LayoutObjectName(std::string name)
    : name_{ObjectName{std::move(name)}} {}

    LayoutObjectName(std::vector<std::string>&& scope)
    : name_{ObjectName{std::move(scope)}} {}

    inline LayoutObjectName& operator=(const LayoutObjectName& o) noexcept {
        this->name_ = o.name_;
        return *this;
    }

    inline std::string toString() const noexcept {
        return name_.toString();
    }

    // inline bool isVoid() const noexcept {
    //     return exprType_ == LayoutObjectType::Void;
    // }

    // inline bool isEmpty() const noexcept {
    //     return isVoid() &&
    //         std::visit(overloaded{
    //             [](const ScopedName& a) { return a.isEmpty(); },
    //             [](auto&&) { return true; }
    //         }
    //     , value_);
    // }

    // inline
    // std::string toString() const noexcept {
    //     return std::visit(
    //         overloaded{
    //             [](const ScopedName& a) noexcept -> std::string { return a.fullname(); },
    //             [](auto&&) noexcept -> std::string { return ""; }
    //         }
    //     , value_);
    // }
};

class LayoutValueExpression;
enum class LayoutObjectType {
    Value,
    Name,

    Refer,

    Return,
    DefineVariable,
    SetValue,

    CallFunction,
    DefineFunction,

    OpNegate
};

// LayoutObjectType::Refer, type/value
using LayoutReferName = LayoutObjectName;
// class LayoutReferName {
//     LayoutObjectName refee_;

// public:
//     LayoutReferName(LayoutObjectName obj)
//     : refee_{std::move(obj)} {}
// };

// LayoutObjectType::Refer, value
class LayoutReferOf {
    std::variant<LayoutReferName, std::unique_ptr<LayoutReferOf>> parent_;
    LayoutReferName child_;
public:
    LayoutReferOf(const LayoutReferOf& r)
    : parent_{std::visit(overloaded{
        [](const LayoutReferName& name) -> decltype(parent_) { return name; },
        [](const std::unique_ptr<LayoutReferOf>& ref) -> decltype(parent_) {
            return std::make_unique<LayoutReferOf>(*ref);
        }
    }, r.parent_)}
    , child_{r.child_}
    {}

    LayoutReferOf(LayoutReferName parent, LayoutReferName child)
    : parent_{std::move(parent)}
    , child_{std::move(child)} {}

    LayoutReferOf(const LayoutReferOf& parent, LayoutReferName child)
    : parent_{std::make_unique<LayoutReferOf>(parent)}
    , child_{std::move(child)} {}

    std::string toString() const noexcept {
        return std::visit(overloaded {
            [](const LayoutReferName& name) noexcept -> std::string {
                return name.toString();
            },
            [](const std::unique_ptr<LayoutReferOf>& of) noexcept -> std::string {
                return of->toString();
            }
        }, parent_) + "." + child_.toString();
    }
};

class LayoutRefer {
    std::variant<LayoutReferName, LayoutReferOf> ref_;

public:
    LayoutRefer(const LayoutRefer& r)
    : ref_{r.ref_} {}

    explicit LayoutRefer(LayoutRefer&& r) : ref_{std::move(r.ref_)} {}

    LayoutRefer(LayoutReferName ref)
    : ref_{std::move(ref)} {}

    LayoutRefer(LayoutReferOf ref)
    : ref_{std::move(ref)} {}

    LayoutRefer& operator=(const LayoutRefer&) noexcept;

    LayoutReferOf asOf(const LayoutReferName& child) noexcept {
        return (LayoutReferOf)std::visit(
            [&child](const auto& of) noexcept -> LayoutReferOf {
                return LayoutReferOf{of, child};
            }
            , ref_);
    }

    std::string toString() const noexcept {
        return std::visit(overloaded {
            [](const LayoutReferName& name) noexcept {
                return name.toString();
            },
            [](const LayoutReferOf& of) noexcept {
                return of.toString();
            }
        }, ref_);
    }
};

// type-name
class LayoutTypeExpression {
    std::variant<LayoutObjectName, LayoutRefer> type_;

public:
    LayoutTypeExpression(const LayoutTypeExpression& e)
    : type_{e.type_}
    {}

    LayoutTypeExpression(std::string name) : type_{LayoutObjectName(std::move(name))} {}
    LayoutTypeExpression(const LayoutRefer& ref) : type_{ref} {}

    inline
    std::string toString() const noexcept {
        return std::visit(overloaded {
            [](LayoutObjectName name) noexcept -> std::string {
                return name.toString();
            },
            [](LayoutRefer ref) noexcept -> std::string {
                return ref.toString();
            }
        }, type_);
    }
};

class LayoutCall {
    LayoutRefer callee_;
    std::vector<std::unique_ptr<LayoutValueExpression>> args_;

public:
    LayoutCall(const LayoutCall& c) noexcept
    : callee_(c.callee_)
    , args_{c.args_.size()}
    {
        std::transform(c.args_.begin(), c.args_.end(), std::back_inserter(args_),
            [](const std::unique_ptr<LayoutValueExpression>& p) noexcept {
                return std::make_unique<LayoutValueExpression>(*p);
            });
    }

    LayoutCall(LayoutRefer, std::vector<LayoutValueExpression>&&) noexcept;

    inline
    std::string toString() const noexcept {
        return "<call>: " + callee_.toString();
    }

    LayoutCall& operator=(const LayoutCall& c) noexcept;

    LayoutCall& operator=(LayoutCall&& c) noexcept;
};

template <typename T>
class LayoutReturn {
//     std::unique_ptr<LayoutValueExpression> expr_;

// public:
//     explicit LayoutReturn(LayoutValueExpression&& expr)
//     : expr_{std::make_unique<LayoutValueExpression>(std::move(expr))}
//     {}
public:
    LayoutReturn() {}

    LayoutReturn(const LayoutReturn<T>& r) = default;
};

// LayoutObjectType::Return
class LayoutReturnVoid : LayoutReturn<LayoutReturnVoid> {
public:
    LayoutReturnVoid() : LayoutReturn<LayoutReturnVoid>{} {}
    LayoutReturnVoid(const LayoutReturnVoid&) = default;

    inline constexpr
    std::string toString() const noexcept {
        return "<return>: void";
    }

    LayoutReturnVoid& operator=(const LayoutReturnVoid&) noexcept;
};

// LayoutObjectType::Return
class LayoutReturnValue : LayoutReturn<LayoutReturnValue> {
    std::unique_ptr<LayoutValueExpression> expr_;
public:
    explicit LayoutReturnValue(LayoutCall&& c);

    LayoutReturnValue(const LayoutReturnValue& v)
    : LayoutReturn<LayoutReturnValue>(static_cast<LayoutReturn>(v))
    , expr_{std::make_unique<LayoutValueExpression>(*v.expr_)} {}

    LayoutReturnValue(LayoutValueExpression expr);

    std::string toString() const noexcept;

    LayoutReturnValue& operator=(const LayoutReturnValue& v) noexcept;
};

class LayoutOpNegate {
    std::unique_ptr<LayoutValueExpression> expr_;

public:
    LayoutOpNegate(const LayoutOpNegate& o) noexcept
    : expr_{std::make_unique<LayoutValueExpression>(*o.expr_)} {}

    LayoutOpNegate(LayoutOpNegate&& o) noexcept
    : expr_{std::move(o.expr_)} {}

    LayoutOpNegate(const LayoutCall& expr) noexcept;
    LayoutOpNegate(LayoutValueExpression&& e)
    : expr_{std::make_unique<LayoutValueExpression>(std::move(e))} {}

    ; // TODO primitive boolean

    inline
    std::string toString() const noexcept {
        return "<opNegate>";
    }

    LayoutOpNegate& operator=(const LayoutOpNegate& o) noexcept;
};

class LayoutOpAnd {
    std::unique_ptr<LayoutValueExpression> expr1_;
    std::unique_ptr<LayoutValueExpression> expr2_;

public:
    LayoutOpAnd(const LayoutOpAnd& o)
    : expr1_{std::make_unique<LayoutValueExpression>(*o.expr1_)}
    , expr2_{std::make_unique<LayoutValueExpression>(*o.expr2_)} {}

    LayoutOpAnd(LayoutOpAnd&& o)
    : expr1_{std::move(o.expr1_)}
    , expr2_{std::move(o.expr2_)} {}

    LayoutOpAnd(LayoutValueExpression expr1, LayoutValueExpression expr2);

    inline
    std::string toString() const noexcept {
        return "<opAnd>";
    }

    LayoutOpAnd& operator=(const LayoutOpAnd& o) noexcept;
};

class LayoutOpOr {
    std::unique_ptr<LayoutValueExpression> expr1_;
    std::unique_ptr<LayoutValueExpression> expr2_;

public:
    LayoutOpOr(const LayoutOpOr& o)
    : expr1_{std::make_unique<LayoutValueExpression>(*o.expr1_)}
    , expr2_{std::make_unique<LayoutValueExpression>(*o.expr2_)} {}

    LayoutOpOr(LayoutOpOr&&);

    LayoutOpOr(LayoutValueExpression, LayoutValueExpression);

    inline
    std::string toString() const noexcept {
        return "<opOr>";
    }

    LayoutOpOr& operator=(const LayoutOpOr& o) noexcept;
};

class LayoutOpSet {
    // using set_element_t = std::variant<LayoutRefer,
    //     LayoutOpAnd, LayoutOpOr, LayoutOpNegate, LayoutOpSet>;

    LayoutRefer refee_;
    const std::unique_ptr<LayoutValueExpression> expr_;
public:
    LayoutOpSet(const LayoutOpSet& o) noexcept
    : refee_{o.refee_}
    , expr_{std::make_unique<LayoutValueExpression>(*o.expr_)} {}

    explicit LayoutOpSet(LayoutRefer&& r, LayoutValueExpression&& expr)
    : refee_{std::move(r)}
    , expr_{std::make_unique<LayoutValueExpression>(std::move(expr))}
    {}

    auto expectedValue() const noexcept;

    inline
    std::string toString() const noexcept {
        return "<opSet>";
    }

    inline
    LayoutOpSet& operator=(const LayoutOpSet& o) noexcept {
        this->refee_ = o.refee_;
        *const_cast<std::unique_ptr<LayoutValueExpression>*>(&expr_)
            = std::make_unique<LayoutValueExpression>(*o.expr_);
        return *this;
    }
};

// class LayoutOp {
//     std::variant<std::>
// };

class LayoutEntityVariable;
// return / op / of / call
class LayoutValueExpression {
    struct L0 {
        L0(LayoutReturnVoid) {}
    };
    struct L1 {
        L1(LayoutRefer) {}
        L1(LayoutOpNegate) {}
    };
    struct L2 {
        L2(LayoutOpAnd) {}
        L2(LayoutOpOr) {}
        L2(LayoutReturnValue) {}
    };
    struct LN {
        LN(LayoutCall) {}
    };

    std::variant<LayoutReturnVoid, LayoutRefer
        , LayoutOpNegate, LayoutOpAnd, LayoutOpOr, LayoutOpSet
        , LayoutReturnValue, LayoutCall
        , std::shared_ptr<LayoutEntityVariable>> expr_;
public:
    LayoutValueExpression(const LayoutValueExpression& e);
    // : expr_{e.expr_} {}

    LayoutValueExpression(LayoutValueExpression&& e);
    // : expr_{std::move(e.expr_)} {}

    LayoutValueExpression(LayoutReturnVoid&& r) noexcept;
    LayoutValueExpression(LayoutEntityVariable&&) noexcept;

    LayoutValueExpression(LayoutRefer&& ref) noexcept;
    LayoutValueExpression(LayoutOpNegate&& op) noexcept;

    LayoutValueExpression(LayoutOpAnd&& op) noexcept;
    LayoutValueExpression(LayoutOpOr&& op) noexcept;
    LayoutValueExpression(LayoutOpSet&&) noexcept;
    LayoutValueExpression(LayoutReturnValue&& r) noexcept;

    LayoutValueExpression(LayoutCall&&) noexcept;

    inline
    auto visit(auto&& f) noexcept {
        return std::visit(std::forward<decltype(f)>(f), expr_);
    }

    std::string toString() const noexcept;

    LayoutValueExpression& operator=(const LayoutValueExpression&) noexcept;
};

class LayoutExpression {
    std::variant<LayoutTypeExpression, LayoutValueExpression>;
public:
    ;
};

class LayoutEntity {
protected:
    LayoutObjectName name_;
public:
    LayoutEntity(const LayoutEntity& e) : name_{e.name_} {}
    LayoutEntity(LayoutEntity&& e) : name_{std::move(e.name_)} {}

    LayoutEntity(LayoutObjectName name) : name_{std::move(name)} {}
};

class LayoutEntityAttributes {
    std::vector<LayoutObjectName> attr_;

public:
    LayoutEntityAttributes(const LayoutEntityAttributes& a)
    : attr_{a.attr_}
    {}

    LayoutEntityAttributes(std::vector<std::string>&& vAttr) noexcept {
        for(auto it = vAttr.begin(); it != vAttr.end(); ++it)
            attr_.emplace_back(std::move(*it));
    }

    inline
    LayoutEntityAttributes& operator=(const LayoutEntityAttributes& a) noexcept {
        attr_ = std::move(a.attr_);
        return *this;
    }
};

class LayoutEntityVariable : LayoutEntity {
    std::optional<LayoutEntityAttributes> attr_;
    LayoutTypeExpression type_;
    std::optional<LayoutValueExpression> expr_;

public:
    LayoutEntityVariable(const LayoutEntityVariable& e)
    : LayoutEntity(static_cast<const LayoutEntity&>(e))
    , attr_{e.attr_}
    , type_{e.type_}
    , expr_{e.expr_}
    {}

    LayoutEntityVariable(LayoutObjectName name, LayoutTypeExpression type,
        std::optional<LayoutEntityAttributes> attr = {})
    : LayoutEntity{std::move(name)}
    , type_{std::move(type)}
    , attr_{std::move(attr)} {}

    LayoutEntityVariable(LayoutObjectName name, LayoutTypeExpression type,
        LayoutValueExpression expr, std::optional<LayoutEntityAttributes> attr = {})
    : LayoutEntity{std::move(name)}
    , type_{std::move(type)}
    , expr_{std::move(expr)}
    , attr_{std::move(attr)} {}

    inline
    LayoutEntityVariable& operator=(const LayoutEntityVariable& v) noexcept {
        this->name_ = v.name_;
        this->type_ = v.type_;
        
        if(v.expr_) this->expr_ = *v.expr_;
        else this->expr_ = {};

        this->attr_ = v.attr_;
        return *this;
    }

    inline
    std::string toString() const noexcept {
        const auto& name = name_.toString();
        const auto& type = type_.toString();

        if(attr_) ;

        if(expr_) ;

        return name + ": " + type;
    }
};

class LayoutEntityFunction : LayoutEntity {
    std::optional<LayoutEntityAttributes> attr_;
    LayoutRefer rType_;
    std::vector<LayoutEntityVariable> args_;

public:
    LayoutEntityFunction(const LayoutEntityFunction& f)
    : LayoutEntity(static_cast<const LayoutEntity&>(f))
    , attr_{f.attr_}
    , rType_{f.rType_}
    , args_{f.args_}
    {}

    LayoutEntityFunction(LayoutObjectName name, LayoutRefer ret,
        std::vector<LayoutEntityVariable> args, std::optional<LayoutEntityAttributes> attr = {})
    : LayoutEntity{std::move(name)}
    , rType_{std::move(ret)}
    , args_{std::move(args)}
    , attr_{std::move(attr)} {}

    inline
    std::string brief() const noexcept {
        const auto name = name_.toString();
        const auto ret = rType_.toString();
        std::string args;

        // std::transform(args_.begin(), args_.end(), std::back_inserter(args)
        //     , [](const LayoutEntityVariable& v) { return v.toString(); });

        for(auto it = args_.begin(); it != args_.end(); ++it) {
            args += it->toString();
            
            if(std::next(it) != args_.end())
                args += ", ";
        }

        if(args_.empty()) return '<' + name + ">(void) -> " + ret;
        return '<' + name + ">(" + args + ") -> " + ret;
    }

    inline
    LayoutEntityFunction& operator=(const LayoutEntityFunction& f) noexcept {
        this->name_ = f.name_;
        this->rType_ = f.rType_;
        this->args_ = f.args_;
        this->attr_ = f.attr_;
        return *this;
    }
};

class LayoutEntityClass : LayoutEntity {
// protected:
    std::optional<LayoutEntityAttributes> attr_;
    std::vector<LayoutEntityVariable> cMembers_;
    std::vector<LayoutEntityFunction> cMethods_;

public:
    LayoutEntityClass(const LayoutEntityClass& c)
    : LayoutEntity(static_cast<LayoutEntity>(c))
    , attr_{c.attr_}
    , cMembers_{c.cMembers_}
    , cMethods_{c.cMethods_}
    {}

    LayoutEntityClass(LayoutObjectName name
        , std::vector<LayoutEntityVariable>&& members
        , std::vector<LayoutEntityFunction>&& methods
        , std::optional<LayoutEntityAttributes> attrs = {})
    : LayoutEntity(std::move(name))
    , cMembers_{std::move(members)}
    , cMethods_{std::move(methods)}
    , attr_{std::move(attrs)}
    {}

    inline
    LayoutEntityClass& operator=(const LayoutEntityClass& c) noexcept {
        this->name_ = c.name_;
        this->cMembers_ = c.cMembers_;
        this->cMethods_ = c.cMethods_;
        this->attr_ = c.attr_;
        return *this;
    }

    inline
    std::string brief() const noexcept {
        const auto& name = name_.toString();
        
        // TODO

        return '<' + name + ">: {}";
    }
};

class LayoutBuilder {
public:
    static
    LayoutRefer refer(LayoutObjectName obj) noexcept {
        LayoutRefer ref{std::move(obj)};
        return ref;
    }

    static
    LayoutRefer buildOf(LayoutRefer parent, LayoutReferName child) noexcept {
        // LayoutRefer of{std::move(parent), std::move(child)};
        // return LayoutRefer{std::move(of)};
        return parent.asOf(std::move(child));
    }

    static
    auto buildReturn() noexcept {
        return LayoutReturnVoid{};
    }

    // static
    // auto buildReturn(LayoutCall c) noexcept {
    //     LayoutReturn ret{std::move(c)};
    //     return std::move(ret);
    // }

    static
    auto buildReturn(LayoutValueExpression&& expr) noexcept {
        LayoutReturnValue ret{std::move(expr)};
        return std::move(ret);
    }

    static
    LayoutOpSet opSet(LayoutRefer r, LayoutValueExpression e) noexcept {
        return LayoutOpSet{std::move(r), std::move(e)};
    }

    static
    auto opAnd(LayoutValueExpression e1, LayoutValueExpression e2) noexcept {
        return LayoutOpAnd{std::move(e1), std::move(e2)};
    }

    static
    auto opNegate(LayoutCall c) noexcept {
        LayoutOpNegate op{std::move(c)};
        return std::move(op);
    }

    static
    auto opNegate(LayoutValueExpression&& e) noexcept {
        return LayoutOpNegate{std::move(e)};
    }

    static
    auto defineVariable(LayoutObjectName name, LayoutTypeExpression type, std::optional<LayoutEntityAttributes> attr = {}) noexcept {
        LayoutEntityVariable var{std::move(name), std::move(type), std::move(attr)};
        return std::move(var);
    }

    static
    auto defineVariable(LayoutObjectName name, LayoutTypeExpression type, LayoutValueExpression expr, std::optional<LayoutEntityAttributes> attr = {}) noexcept {
        LayoutEntityVariable var{std::move(name), std::move(type), std::move(expr), std::move(attr)};
        return std::move(var);
    }

    static
    auto declareFunction(LayoutObjectName name, LayoutRefer ret, std::vector<LayoutEntityVariable> args, std::optional<LayoutEntityAttributes> attr = {}) noexcept {
        LayoutEntityFunction fn{std::move(name), std::move(ret), std::move(args), std::move(attr)};
        return std::move(fn);
    }
};

// using type_t = ast::Type;
// using expr_t = parser::expr_t;
// using metavars_t = parser::metavars_t;
// using arglist_t = std::vector<std::pair<type_t, std::string>>;


// // NOTE: Once thought about layout concept;
// // Which preceded arranging data/types; Layout -> Context -> AST
// struct Layout {};

// struct LAttrs : Layout {
//     std::vector<std::string> vAttrs_;
//     LAttrs() : Layout(), vAttrs_{} {}
// };

// struct LFunction : Layout {
//     LAttrs fAttrs_;
//     std::string fName_;
//     metavars_t fMVars_;
//     arglist_t fArgs_;
//     std::vector<expr_t> fExprs_;

//     LFunction(std::string fName, metavars_t fMVars,
//         arglist_t fArgs, std::vector<expr_t> fExprs, LAttrs&& fAttrs = {}) : Layout(),
//         fName_(fName), fMVars_(fMVars), fArgs_(fArgs),
//         fExprs_(fExprs), fAttrs_{std::move(fAttrs)} {}
// };

// struct LClass : Layout {
//     using members_t = std::vector<std::pair<std::string, expr_t>>;

//     LAttrs cAttrs_;
//     std::string cName_;
//     metavars_t cMVars_;
//     members_t cMembers_;
//     std::vector<LFunction> cMethods_;

//     LClass(std::string cName, metavars_t cMVars,
//         members_t cMembers, std::vector<LFunction> cMethods, LAttrs&& cAttrs = {}) : Layout(),
//         cName_{cName}, cMVars_{cMVars}, cMembers_{cMembers}, cMethods_{cMethods},
//         cAttrs_{std::move(cAttrs)} {}
// };

NSRCXEND

#endif // RCX_PARSE_LAYOUT_LAYOUT_HPP