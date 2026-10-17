#include "Context.hpp"

#include <parse/Layout/Layout.hpp>

NSRCXBGN

namespace ctx {
// void
// SpaceContext::addDef(context_t && rhs) noexcept {
//   this->ctx_.push_back(rhs);
// }

unsigned
SpaceContext::dumpDef() const noexcept {
    if(ctx_.empty()) return 0;

    unsigned sz = ctx_.size();
    std::vector<std::string> vDef(sz);

    for(auto& v : ctx_) {
        vDef.emplace_back(
            std::visit<std::string>(
                overloaded {
                    // [](const std::shared_ptr<ExpressionContext>&) { return "ExpressionContext"; },
                    [](const std::shared_ptr<SpaceContext>&) { return "SpaceContext"; },
                    [](const std::shared_ptr<LocalSpaceContext>&) { return "LocalSpaceContext"; },
                    // [](const std::shared_ptr<ClassContext>&) { return "ClassContext"; },
                    // [](const std::shared_ptr<FunctionContext>& fc) {
                    //     return "[FunctionContext]:\t" + fc->dumpDef();
                    // }
                    [](const std::shared_ptr<LayoutEntityClass>& c) {
                        return "[Class]:\t" + c->brief();
                    },
                    [](const std::shared_ptr<LayoutEntityFunction>& f) {
                        return "[Function]:\t" + f->brief();
                    }
                }, v));
        spdlog::debug("dumpDef: {}", vDef.back());
    }

    return sz;
}

unsigned
LocalSpaceContext::dumpDef() const noexcept {
    if(ctx_.empty()) return 0;

    unsigned sz = ctx_.size();
    std::vector<std::string> vDef(sz);

    for(auto& v : ctx_) {
        vDef.emplace_back(
            std::visit<std::string>(
                overloaded {
                    [](const std::shared_ptr<LocalSpaceContext>&) { return "LocalSpaceContext"; },
                    [](const std::shared_ptr<LayoutValueExpression>& e) noexcept {
                        return "[Expr]:\t" + e->toString();
                    }
                }
            , v)
        );

        spdlog::debug("dumpDef: {}", vDef.back());
    }

    return sz;
}

} // ns ctx

NSRCXEND
