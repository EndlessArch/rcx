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
#include "clang/Frontend/CommandLineSourceLoc.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/FrontendOptions.h"
#include "clang/Lex/HeaderSearchOptions.h"
#include "clang/Lex/ModuleLoader.h"
#include "clang/Sema/CodeCompleteConsumer.h"
#include "clang/Sema/DeclSpec.h"
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
    CompilerInvocation::CreateFromArgs(*invc, {
        // "-isysroot",
        // "/Library/Developer/CommandLineTools/SDKs/MacOSX12.3.sdk",
        "-I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1",
        "-I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include",
    }, diag);

    invc->getDiagnosticOpts().ShowColors = 1;

    auto & fe = invc->getFrontendOpts();
    auto f = FrontendInputFile(sourceName, Language::CXX);

    // is this required ?
    invc->PreprocessorOpts = std::make_shared<PreprocessorOptions>();

    invc->getHeaderSearchOpts().UseLibcxx = 1;
    invc->getHeaderSearchOpts().UseStandardSystemIncludes = 1;

    invc->getLangOpts()->IncludeDefaultHeader = 1;

    // invc->getHeaderSearchOpts().AddPath(
    //     "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
    //     frontend::IncludeDirGroup::System, false, true);

//     invc->getHeaderSearchOpts().AddPath(
// #ifdef __APPLE__
//         "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
// #endif
//         "/usr/include/c++/v1",
//         frontend::IncludeDirGroup::System, false, false);

//     invc->getHeaderSearchOpts().AddPath(
// #ifdef __APPLE__
//         "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
// #endif
//         "/usr/include",
//         frontend::IncludeDirGroup::System, false, false);

// #ifdef __APPLE__
//     invc->getHeaderSearchOpts().AddPath(
//     "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
//     frontend::IncludeDirGroup::IndexHeaderMap, true, false);
// #endif

    invc->getHeaderSearchOpts().UseStandardSystemIncludes = 1;
    invc->getHeaderSearchOpts().UseStandardCXXIncludes = 1;
    invc->getHeaderSearchOpts().UseBuiltinIncludes = 1;
    // invc->getHeaderSearchOpts().Sysroot = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/";
    // invc->getHeaderSearchOpts().ResourceDir = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/";

    fe.ProgramAction = frontend::ASTDeclList;
    fe.Inputs.clear(); // remove '-', stdin
    fe.Inputs.push_back(f);

    CompilerInstance clang;

    clang.setInvocation(std::move(invc));
    clang.createDiagnostics();

    clang.createTarget();

    llvm::vfs::OverlayFileSystem ofs(llvm::vfs::getRealFileSystem());
    ofs.openFileForRead(sourceName);

    auto fm = llvm::makeIntrusiveRefCnt<FileManager>(FileSystemOptions(),
        llvm::makeIntrusiveRefCnt<llvm::vfs::OverlayFileSystem>(ofs));
    clang.createSourceManager(*fm);
    clang.setFileManager(fm.get());

    spdlog::info("code:\n{}", fm->getVirtualFileSystem().getBufferForFile(std::move(sourceName)).get()->getBuffer().data());

    // auto & sm = clang.getSourceManager();
    // sm.setMainFileID(sm.createFileID(
    //     fm->getVirtualFileSystem().getBufferForFile(sourceName).get()->getMemBufferRef()));

    clang.createPreprocessor(TranslationUnitKind::TU_Complete);

    // clang.createASTReader();
    // clang.getFrontendOpts().CodeCompletionAt.FileName = sourceName;

    // clang.createCodeCompletionConsumer();
    CodeCompleteConsumer * ccc = nullptr;
    if(clang.hasCodeCompletionConsumer())
        ccc = &clang.getCodeCompletionConsumer();
    // clang.createSema(TranslationUnitKind::TU_Complete, ccc);

    // clang::ParseAST(clang.getSema());

    // ASTDumper dumper(llvm::errs(), true);
    // dumper.dumpLookups(clang.getSema().getFunctionLevelDeclContext(), true);

    GeneratePCHAction act;
    // PrintPreambleAction act;
    // ExtractAPIAction act;
    clang.ExecuteAction(act);

    // auto & ctx = clang.getASTContext();
    // DeclaratorChunk::FunctionTypeInfo fti;

    return Package<ctx::context_t>::makeBroken("Failed task.",
        []() noexcept { return ctx::ModuleContext({}); });
}

Package<ctx::context_t>
parseModule(void) noexcept {
}

NSRCXEND