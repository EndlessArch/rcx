#include <iostream>
#include <memory>

#include <parse/parser.hpp>
#include <conv/Modernizer.hpp>

#ifdef __cplusplus
extern "C"
#endif
int rcx_main(
    int argc,
    char const* const* argv) {
    ;

    auto ctx = rcx::parseStart(argc, (char **)argv)();

    std::get<rcx::ctx::ModuleContext>(ctx);

    return 0;
}