#include "Layout.hpp"

NSRCXBGN

LayoutCall::LayoutCall(LayoutRefer callee, std::vector<LayoutValueExpression>&& args) noexcept
: callee_{std::move(callee)}
, args_{args.size()} {
    std::transform(args.begin(), args.end(), std::back_inserter(args_),
        [](LayoutValueExpression v) {
            return std::make_unique<LayoutValueExpression>(std::move(v)); });
}

LayoutCall& LayoutCall::operator=(const LayoutCall& c) noexcept {
    this->callee_ = c.callee_;

    this->args_.clear();
    for(auto it = c.args_.begin(); it != c.args_.end(); ++it)
        this->args_.push_back(std::make_unique<LayoutValueExpression>(**it));

    return *this;
}

LayoutCall& LayoutCall::operator=(LayoutCall&& c) noexcept {
    this->callee_ = std::move(c.callee_);
    this->args_ = std::move(c.args_);
    return *this;
}

LayoutRefer& LayoutRefer::operator=(const LayoutRefer& o) noexcept {
    std::visit(overloaded {
        [this](const LayoutReferName& r) noexcept {
            ref_.emplace<0>(r);
        },
        [this](const LayoutReferOf& o) noexcept {
            ref_.emplace<1>(o);
        }
    }, o.ref_);
    return *this;
}

LayoutReturnVoid& LayoutReturnVoid::operator=(const LayoutReturnVoid&) noexcept {
    return *this;
}

LayoutReturnValue::LayoutReturnValue(LayoutCall&& c)
: LayoutReturn<LayoutReturnValue>{}
, expr_{std::make_unique<LayoutValueExpression>(std::move(c))} {}

LayoutReturnValue::LayoutReturnValue(LayoutValueExpression expr)
: expr_{std::make_unique<LayoutValueExpression>(std::move(expr))}
{}

std::string LayoutReturnValue::toString() const noexcept {
    return "<return>: " + expr_->toString();
}

LayoutReturnValue& LayoutReturnValue::operator=(const LayoutReturnValue& v) noexcept {
    this->expr_ = std::make_unique<LayoutValueExpression>(*v.expr_);
    return *this;
}

LayoutOpNegate::LayoutOpNegate(const LayoutCall& expr) noexcept
: expr_{std::make_unique<LayoutValueExpression>(std::move(expr))} {}

LayoutOpNegate& LayoutOpNegate::operator=(const LayoutOpNegate& o) noexcept {
    this->expr_ = std::make_unique<LayoutValueExpression>(*o.expr_);
    return *this;
}

LayoutOpAnd::LayoutOpAnd(LayoutValueExpression expr1, LayoutValueExpression expr2)
: expr1_{std::make_unique<LayoutValueExpression>(std::move(expr1))}
, expr2_{std::make_unique<LayoutValueExpression>(std::move(expr2))}
{}

LayoutOpAnd& LayoutOpAnd::operator=(const LayoutOpAnd& o) noexcept {
    this->expr1_ = std::make_unique<LayoutValueExpression>(*o.expr1_);
    this->expr2_ = std::make_unique<LayoutValueExpression>(*o.expr2_);
    return *this;
}

LayoutOpOr::LayoutOpOr(LayoutOpOr&& o)
: expr1_{std::move(o.expr1_)}
, expr2_{std::move(o.expr2_)} {}

LayoutOpOr::LayoutOpOr(LayoutValueExpression expr1, LayoutValueExpression expr2)
: expr1_{std::make_unique<LayoutValueExpression>(std::move(expr1))}
, expr2_{std::make_unique<LayoutValueExpression>(std::move(expr2))}
{}

LayoutOpOr& LayoutOpOr::operator=(const LayoutOpOr& o) noexcept {
    this->expr1_ = std::make_unique<LayoutValueExpression>(*o.expr1_);
    this->expr2_ = std::make_unique<LayoutValueExpression>(*o.expr2_);
    return *this;
}

auto LayoutOpSet::expectedValue() const noexcept {
    // return std::visit([](const auto& e) -> LayoutValueExpression {
    //     return LayoutValueExpression{e};
    // }, expr_);
    return *expr_;
}

LayoutValueExpression::LayoutValueExpression(const LayoutValueExpression& e)
: expr_{e.expr_} {}
// : expr_{std::visit(
// [](const auto& a) -> decltype(expr_) {
//     using a_type = typename std::decay<decltype(a)>;
//     using r_type = typename std::decay<decltype(expr_)>;

//     if constexpr (rcx::is_kind_of_v<std::shared_ptr, a_type>) {
//         auto v = std::make_shared<LayoutEntityVariable>();
//         return v;
//     }
    
//     return a;
// }, e.expr_)} {}

LayoutValueExpression::LayoutValueExpression(LayoutValueExpression&& e)
: expr_{std::move(e.expr_)} {}

LayoutValueExpression::LayoutValueExpression(LayoutReturnVoid&& r) noexcept
: expr_{std::move(r)} {}

LayoutValueExpression::LayoutValueExpression(LayoutEntityVariable&& v) noexcept
: expr_{std::make_shared<LayoutEntityVariable>(std::move(v))} {}

LayoutValueExpression::LayoutValueExpression(LayoutRefer&& ref) noexcept
: expr_{std::move(ref)} {}

LayoutValueExpression::LayoutValueExpression(LayoutOpNegate&& op) noexcept
: expr_{std::move(op)} {}

LayoutValueExpression::LayoutValueExpression(LayoutOpAnd&& op) noexcept
: expr_{std::move(op)} {}

LayoutValueExpression::LayoutValueExpression(LayoutOpOr&& op) noexcept
: expr_{std::move(op)} {}

LayoutValueExpression::LayoutValueExpression(LayoutOpSet&& op) noexcept
: expr_{std::move(op)} {}

LayoutValueExpression::LayoutValueExpression(LayoutReturnValue&& r) noexcept
: expr_{std::move(r)} {}

LayoutValueExpression::LayoutValueExpression(LayoutCall&& c) noexcept
: expr_{std::move(c)} {}

std::string LayoutValueExpression::toString() const noexcept {
    return std::visit([](const auto& a) noexcept {
        if constexpr(rcx::is_kind_of_v<std::shared_ptr, decltype(a)>) {
            return a->toString();
        }
        else return a.toString();
    }, this->expr_);
}

LayoutValueExpression& LayoutValueExpression::operator=(const LayoutValueExpression& e) noexcept {
    std::visit([this](const auto& a) noexcept {
        this->expr_.operator=<decltype(a)>(a);
    }, e.expr_);
    return *this;
}

NSRCXEND