#include "parser.hpp"

#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/AST/ASTDumper.h>
#include <clang/Parse/Parser.h>
#include <clang/Sema/Sema.h>
#include "clang-c/Index.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContextAllocate.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclGroup.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/FileEntry.h"
#include "clang/Basic/FileSystemOptions.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/FrontendOptions.h"
#include "clang/Sema/CodeCompleteConsumer.h"
#include "clang/Serialization/PCHContainerOperations.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>

// #include <ranges>
#include <memory>
#include <parse/CTX/Context.hpp>
#include <conv/Modernizer.hpp>

#include <boost/program_options/variables_map.hpp>

#include <clang/Basic/CodeGenOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/CodeGen/ModuleBuilder.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/FrontendTool/Utils.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/TextDiagnosticBuffer.h>
#include <clang/Parse/ParseAST.h>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Triple.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>

#include <spdlog/spdlog.h>


NSRCXBGN

using namespace clang;

Package<ctx::context_t>
parseStart(llvm::StringMap<boost::program_options::variable_value> && optMap) noexcept {
    
    std::string sourceName = optMap["source"].as<std::string>();
    std::string destName = optMap["-o"].empty() ? "" : optMap["-o"].as<std::string>();

    DiagnosticsEngine diag(
        llvm::IntrusiveRefCntPtr<DiagnosticIDs>(),
        &*llvm::IntrusiveRefCntPtr<DiagnosticOptions>());

    auto invc = std::make_shared<CompilerInvocation>();
    CompilerInvocation::CreateFromArgs(*invc, {}, diag);

    invc->getDiagnosticOpts().ShowColors = 1;

    auto & fe = invc->getFrontendOpts();
    auto f = FrontendInputFile(sourceName, Language::CXX);

    invc->getHeaderSearchOpts().AddPath(
#ifdef __APPLE__
        "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
#endif
        "/usr/include/c++/v1",
        frontend::IncludeDirGroup::System, false, false);

    invc->getHeaderSearchOpts().AddPath(
#ifdef __APPLE__
        "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
#endif
        "/usr/include",
        frontend::IncludeDirGroup::System, false, true);

    fe.ProgramAction = frontend::ASTDeclList;
    fe.Inputs.clear(); // remove '-', stdin
    fe.Inputs.push_back(f);

    // for(auto x : fe.Inputs)
    //     spdlog::info("File : {}", x.getFile());

    CompilerInstance clang;

    clang.setInvocation(std::move(invc));
    clang.createDiagnostics();

    llvm::vfs::OverlayFileSystem ofs(llvm::vfs::getRealFileSystem());
    ofs.openFileForRead(sourceName);

    auto fm = llvm::makeIntrusiveRefCnt<FileManager>(FileSystemOptions(),
        llvm::makeIntrusiveRefCnt<llvm::vfs::OverlayFileSystem>(ofs));
    clang.createSourceManager(*fm);
    clang.setFileManager(fm.get());

    auto & sm = clang.getSourceManager();

    spdlog::info("code:\n{}", fm->getVirtualFileSystem().getBufferForFile(sourceName).get()->getBuffer().data());

    sm.setMainFileID(sm.createFileID(
        fm->getVirtualFileSystem().getBufferForFile(sourceName).get()->getMemBufferRef()));

    // clang.getSourceManager().dump();

    // clang.createSema(TranslationUnitKind::TU_Complete, nullptr);
    // clang::ParseAST(clang.getSema());

    GeneratePCHAction act;
    clang.ExecuteAction(act);

    // auto mod = compiler.Compile();
    // spdlog::info("Got module (null ? {})", (mod == nullptr));
    // mod->print(llvm::errs(), nullptr);

    return Package<ctx::context_t>::makeBroken("Failed",
        []() noexcept { return ctx::ModuleContext({}); });
}

Package<ctx::context_t>
parseModule(void) noexcept {
}

NSRCXEND