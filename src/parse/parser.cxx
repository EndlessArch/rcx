#include "parser.hpp"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContextAllocate.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclGroup.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileEntry.h"
#include "clang/Sema/CodeCompleteConsumer.h"
#include "llvm/Support/PrettyStackTrace.h"

#include <fstream>
#include <functional>
#include <istream>
#include <iterator>

// #include <ranges>
#include <parse/CTX/Context.hpp>
#include <conv/Modernizer.hpp>

#include <boost/program_options/variables_map.hpp>

#include <clang/Basic/CodeGenOptions.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/CodeGen/ModuleBuilder.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Parse/ParseAST.h>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Triple.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>

#include <spdlog/spdlog.h>


NSRCXBGN

namespace {

using namespace clang;

struct RCXAC : public ASTConsumer{
    std::unique_ptr<CodeGenerator> builder_;
    std::vector<Decl *> topLvlDecls_;

    RCXAC(std::shared_ptr<CodeGenerator> builderIn)
    : ASTConsumer(), builder_(builderIn.get()) {}
    ~RCXAC(void) = default;

    void Initialize(ASTContext&) override;
    bool HandleTopLevelDecl(DeclGroupRef) override;
    void HandleInlineFunctionDefinition(FunctionDecl*) override;
    void HandleInterestingDecl(DeclGroupRef) override;
    void HandleTranslationUnit(ASTContext&) override;
    void HandleTagDeclDefinition(TagDecl*) override;
    void HandleTagDeclRequiredDefinition(const TagDecl*) override;
    void HandleCXXImplicitFunctionInstantiation(FunctionDecl*) override;
    void HandleImplicitImportDecl(ImportDecl*) override;
    void CompleteTentativeDefinition(VarDecl*) override;
    void CompleteExternalDeclaration(VarDecl*) override;
    void AssignInheritanceModel(CXXRecordDecl*) override;
    void HandleCXXStaticMemberVarInstantiation(VarDecl*) override;
    void HandleVTable(CXXRecordDecl*) override;
    ASTMutationListener* GetASTMutationListener(void) override;
    ASTDeserializationListener* GetASTDeserializationListener(void) override;
    void PrintStats(void) override;
};

void RCXAC::Initialize(ASTContext& ctx) {
    builder_->Initialize(ctx);
}

bool RCXAC::HandleTopLevelDecl(DeclGroupRef declGroup) {
    for(auto it = declGroup.begin(); it != declGroup.end(); ++it) {
        topLvlDecls_.push_back(*it);
    }
    return builder_->HandleTopLevelDecl(declGroup);
}

void RCXAC::HandleInlineFunctionDefinition(FunctionDecl * fDecl) {
    builder_->HandleInlineFunctionDefinition(fDecl);
}

void RCXAC::HandleInterestingDecl(DeclGroupRef declGroup) {
    builder_->HandleInterestingDecl(declGroup);
}

void RCXAC::HandleTranslationUnit(ASTContext & ctx) {
    builder_->HandleTranslationUnit(ctx);
}

void RCXAC::HandleTagDeclDefinition(TagDecl * td) {
    builder_->HandleTagDeclDefinition(td);
}

void RCXAC::HandleTagDeclRequiredDefinition(const TagDecl * td) {
    builder_->HandleTagDeclRequiredDefinition(td);
}

void RCXAC::HandleCXXImplicitFunctionInstantiation(FunctionDecl * fDecl) {
    builder_->HandleCXXImplicitFunctionInstantiation(fDecl);
}

void RCXAC::HandleImplicitImportDecl(ImportDecl * iDecl) {
    builder_->HandleImplicitImportDecl(iDecl);
}

void RCXAC::CompleteTentativeDefinition(VarDecl * vDecl) {
    builder_->CompleteTentativeDefinition(vDecl);
}

void RCXAC::CompleteExternalDeclaration(VarDecl * vDecl) {
    builder_->CompleteExternalDeclaration(vDecl);
}

void RCXAC::AssignInheritanceModel(CXXRecordDecl * rDecl) {
    builder_->AssignInheritanceModel(rDecl);
}

void RCXAC::HandleCXXStaticMemberVarInstantiation(VarDecl * vDecl) {
    builder_->HandleCXXStaticMemberVarInstantiation(vDecl);
}

void RCXAC::HandleVTable(CXXRecordDecl * rDecl) {
    builder_->HandleVTable(rDecl);
}

ASTMutationListener * RCXAC::GetASTMutationListener(void) {
    return builder_->GetASTMutationListener();
}

ASTDeserializationListener * RCXAC::GetASTDeserializationListener(void) {
    return builder_->GetASTDeserializationListener();
}

void RCXAC::PrintStats(void) {
    return builder_->PrintStats();
}

struct RCXCompiler {
    llvm::LLVMContext ctx_;
    clang::CompilerInstance clang_;
    std::shared_ptr<clang::CodeGenerator> cg_;
    llvm::Module * mod_ = nullptr;
    unsigned ptrSz_ = 0;

    RCXCompiler(clang::CodeGenOptions cgo = clang::CodeGenOptions()) {
        clang::LangOptions lo;
        lo.CPlusPlus = lo.CPlusPlus17 = 1;
        clang_.getLangOpts() = std::move(lo);
        clang_.getCodeGenOpts() = std::move(cgo);

        std::shared_ptr<DiagnosticConsumer> pDc = std::make_shared<DiagnosticConsumer>();
        pDc->BeginSourceFile(lo);

        clang_.createDiagnostics(pDc.get());

        auto triple = llvm::Triple::normalize(llvm::sys::getDefaultTargetTriple());
        llvm::Triple tr(triple);
        tr.setOS(llvm::Triple::MacOSX);
        tr.setVendor(llvm::Triple::VendorType::Apple);
        tr.setEnvironment(llvm::Triple::EnvironmentType::MacABI);
        clang_.getTargetOpts().Triple = tr.getTriple();
        clang_.setTarget(clang::TargetInfo::CreateTargetInfo(
            clang_.getDiagnostics(),
            std::make_shared<clang::TargetOptions>(clang_.getTargetOpts())
        ));

        const auto & tInf = clang_.getTarget();
        this->ptrSz_ = tInf.getPointerWidth(0) / 8;
        
        clang_.createSourceManager(*clang_.createFileManager());
        clang_.createPreprocessor(clang::TranslationUnitKind::TU_Complete);

        clang_.createASTContext();

        cg_.reset(
            clang::CreateLLVMCodeGen(
                clang_.getDiagnostics(),
                "rcx-main",
                clang_.getHeaderSearchOpts(),
                clang_.getPreprocessorOpts(),
                clang_.getCodeGenOpts(),
                this->ctx_));
    }

    void Initialize(const std::string& filename, std::unique_ptr<ASTConsumer> ac) noexcept {
        clang_.setASTConsumer(std::move(ac));

        clang_.createSema(clang::TU_Complete, nullptr);

        clang::SourceManager& sm = clang_.getSourceManager();

        sm.setMainFileID(sm.createFileID(
            std::move(llvm::MemoryBuffer::getFile(filename).get()),
            clang::SrcMgr::C_User));
        spdlog::info("RCXCompiler initialized");
    }

    auto compile(void) noexcept {

        llvm::EnablePrettyStackTrace();

        spdlog::info((int)clang_.hasASTConsumer());

        clang::ParseAST(clang_.getSema(), true, false);
        // mod_ = static_cast<clang::CodeGenerator&>(clang_.getASTConsumer()).GetModule();

        // auto funcBGN = mod_
        return mod_;
    }
};

} // ns anon

Package<ctx::context_t>
parseStart(llvm::StringMap<boost::program_options::variable_value> && optMap) noexcept {
    
    std::string sourceName = optMap["source"].as<std::string>();
    std::string destName = optMap["-o"].empty() ? "" : optMap["-o"].as<std::string>();

    std::fstream sourceF(sourceName, std::ios_base::in);
    if (!sourceF.is_open()) {
        spdlog::error("Unable to open " + sourceName + ".");
        std::abort();
    }

    RCXCompiler compiler;

    compiler.clang_.getDiagnostics().dump();

    compiler.Initialize(sourceName, std::make_unique<RCXAC>(compiler.cg_));

    compiler.compile();

    // clang::DiagnosticOptions clangOpts;

    // std::unique_ptr<clang::TextDiagnosticPrinter> pClangDiagPrinter
    //  = std::make_unique<clang::TextDiagnosticPrinter>(llvm::errs(), &clangOpts, true);

    // llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> pClangDiagIDs;

    // std::unique_ptr<clang::DiagnosticsEngine> pClangDiag
    //  = std::make_unique<clang::DiagnosticsEngine>(pClangDiagIDs, &clangOpts, pClangDiagPrinter.get());

    // clang::LangOptions langOpts;
    // langOpts.LangStd = clang::LangStandard::lang_cxx17;
    // langOpts.setClangABICompat(clang::LangOptions::ClangABI::Latest);
    // langOpts.setGC(clang::LangOptions::GCMode::NonGC);

    // clang::FileSystemOptions fsOpts;
    // clang::FileManager fMngr(fsOpts);
    // spdlog::info("Got file: {}", fMngr.getFile(sourceName).get()->getName());
    // clang::SourceManager sourceManager(*pClangDiag, fMngr);

    // std::shared_ptr<clang::HeaderSearchOptions>
    //     clangHeaderSearchOpts(new clang::HeaderSearchOptions());
    // clangHeaderSearchOpts->AddPath(
    //     "/usr/local/include",
    //     clang::frontend::Angled,
    //     false, false);
    // clangHeaderSearchOpts->AddPath(
    //     "/usr/local/include/c++/11",
    //     clang::frontend::CXXSystem,
    //     false, false);

    // std::shared_ptr<clang::TargetOptions> tOpts
    //  = std::make_shared<clang::TargetOptions>();
    // tOpts->CPU = llvm::sys::getHostCPUName();
    // tOpts->HostTriple = tOpts->Triple = llvm::sys::getDefaultTargetTriple();

    // std::shared_ptr<clang::TargetInfo> tInf(clang::TargetInfo::CreateTargetInfo(*pClangDiag, tOpts));

    // clang::HeaderSearch clangHeaderSearch(
    //     clangHeaderSearchOpts,
    //     sourceManager,
    //     *pClangDiag,
    //     langOpts, tInf.get());

    // std::unique_ptr<clang::TrivialModuleLoader> clangML
    //  = std::make_unique<clang::TrivialModuleLoader>();

    // // clang::CompilerInstance clang;

    // std::shared_ptr<clang::PreprocessorOptions> pClangPrepropOpts(new clang::PreprocessorOptions());
    // clang::Preprocessor preprop(
    //     pClangPrepropOpts,
    //     *pClangDiag,
    //     langOpts,
    //     sourceManager,
    //     clangHeaderSearch,
    //     *clangML);
    // preprop.Initialize(*tInf);
    // spdlog::info("Preprocessor initialized");

    // clang::RawPCHContainerReader pchReader;

    // clang::FrontendOptions frontendOpts;
    // clang::InitializePreprocessor(
    //     preprop,
    //     *pClangPrepropOpts,
    //     pchReader,
    //     frontendOpts);

    // clang::ApplyHeaderSearchOptions(clangHeaderSearch, *clangHeaderSearchOpts, langOpts, llvm::Triple(tOpts->Triple));

    // clang::IdentifierTable idTab;
    // clang::SelectorTable selTab;
    // clang::Builtin::Context biCtx;
    // biCtx.initializeBuiltins(idTab, langOpts);
    // biCtx.InitializeTarget(*tInf, nullptr);
    // spdlog::info("clang biCtx initialized");

    // clang::ASTContext ctx(
    //     langOpts, sourceManager, idTab, selTab, biCtx,
    //     clang::TranslationUnitKind::TU_Complete);
    // ctx.getTargetInfo().getTargetOpts();

    // clang::ASTConsumer consumer;
    // consumer.Initialize(ctx);

    // clang::Sema sema(preprop, ctx, consumer);
    // sema.Initialize();
    // spdlog::info("clang Sema initialized");

    // clang::Parser parser(preprop, sema, true);
    // // make sure target info isn't null
    // parser.Initialize();
    // spdlog::info("clang Parser initialized");

    // spdlog::info("Parse Top Level Declaration: {}", parser.ParseTopLevelDecl()?"Success":"Failed");

    // spdlog::info("clang::Parser initialized");

    // sema.getASTContext().PrintStats();

    // sema.getASTConsumer().PrintStats();

    return Package<ctx::context_t>::makeBroken("Failed",
    []() noexcept { return ctx::ModuleContext({}); });
}

Package<ctx::context_t>
parseModule(void) noexcept {
}

NSRCXEND