//------------------------------------------------------------------------------
//
// Created by JoaoBispo
//
// Based on public domain code by Eli Bendersky (eliben@gmail.com) -
// http://eli.thegreenplace.net/
//------------------------------------------------------------------------------
#include "ClangAst.h"
#include "../Clava/HandlerCoverage.h"
#include "../ClangAstDumper/ClangAstDumperConstants.h"
#include "ClangNodes.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Comment.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>

#include <cctype>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <string>

using namespace clang;

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

namespace {

constexpr size_t MAX_FATAL_ERROR_MESSAGE_LENGTH = 200;

std::string sanitizeErrorMessage(const char *message) {
    std::string sanitized;

    if (message == nullptr) {
        return sanitized;
    }

    bool lastWasSpace = true;
    for (const char *cursor = message; *cursor != '\0'; ++cursor) {
        if (sanitized.size() >= MAX_FATAL_ERROR_MESSAGE_LENGTH) {
            break;
        }

        if (std::isspace(static_cast<unsigned char>(*cursor))) {
            if (!lastWasSpace) {
                sanitized += ' ';
                lastWasSpace = true;
            }
        } else {
            sanitized += *cursor;
            lastWasSpace = false;
        }
    }

    while (!sanitized.empty() && sanitized.back() == ' ') {
        sanitized.pop_back();
    }

    return sanitized;
}

[[noreturn]] void dumpFatalError(const Decl *D, const char *message) {
    llvm::errs() << "ERROR "
                 << (D == nullptr ? "<unknown>" : clava::getClassName(D)) << " "
                 << sanitizeErrorMessage(message) << "\n";
    llvm::errs().flush();
    clava::reportHandlerCoverage();
    llvm::outs().flush();
    exit(1);
}

} // namespace

static constexpr const char *const PREFIX = "COUNTER";

static llvm::cl::opt<bool> HandlerCoverageReport(
    "handler-coverage-report",
    llvm::cl::desc("Report class names that used fallback dump handling"),
    llvm::cl::init(false));

static llvm::cl::opt<bool> HandlerCoverageAll(
    "handler-coverage-all",
    llvm::cl::desc("Report every class name encountered by each dispatch "
                   "family, not only fallbacks"),
    llvm::cl::init(false));

/* DumpAstVisitor implementation */

bool DumpAstVisitor::TraverseDecl(Decl *D) {
    if (!D) {
        return false;
    }

    FullSourceLoc fullLocation = Context->getFullLoc(D->getBeginLoc());
    if (fullLocation.isValid() && fullLocation.hasManager() &&
        !fullLocation.isInSystemHeader()) {

        // Top-level Node
        llvm::errs() << TOP_LEVEL_NODES << "\n";
        llvm::errs() << D << "_" << id << "\n";
    }

    return false;
}

PrintNodesTypesRelationsVisitor::PrintNodesTypesRelationsVisitor(
    ASTContext *Context, int id, ClangAstDumper dumper)
    : Context(Context), id(id), dumper(dumper){};

// Dump types for Expr, TypeDecl and ValueDecl, as well as the connection
// between them
bool PrintNodesTypesRelationsVisitor::VisitExpr(Expr *D) { return true; }

bool PrintNodesTypesRelationsVisitor::VisitTypeDecl(TypeDecl *D) {
    return true;
}

/**
 * Typedefs will be visited by 'VisitTypeDecl' but will null, this override
 * extracts the correct information for typedefs
 * @param D
 * @return
 */
bool PrintNodesTypesRelationsVisitor::VisitTypedefNameDecl(TypedefNameDecl *D) {
    return true;
}

bool PrintNodesTypesRelationsVisitor::VisitEnumDecl(EnumDecl *D) {
    FullSourceLoc fullLocation = Context->getFullLoc(D->getBeginLoc());
    if (fullLocation.isValid() && !fullLocation.isInSystemHeader()) {
        dumper.VisitTypeTop(D->getIntegerType());
    }

    return true;
}

bool PrintNodesTypesRelationsVisitor::VisitValueDecl(ValueDecl *D) {
    return true;
}

// Visit only nodes from the source code, ignore system headers
bool PrintNodesTypesRelationsVisitor::VisitDecl(Decl *D) {
    FullSourceLoc fullLocation = Context->getFullLoc(D->getBeginLoc());

    if (fullLocation.isValid() && !fullLocation.isInSystemHeader()) {
        dumper.VisitDeclTop(D);
        return true;
    }

    return true;
}

bool PrintNodesTypesRelationsVisitor::VisitStmt(Stmt *D) {
    FullSourceLoc fullLocation = Context->getFullLoc(D->getBeginLoc());

    if (fullLocation.isValid() && !fullLocation.isInSystemHeader()) {
        dumper.VisitStmtTop(D);
        return true;
    }

    return true;
}

bool PrintNodesTypesRelationsVisitor::VisitLambdaExpr(LambdaExpr *D) {
    return true;
}

MyASTConsumer::MyASTConsumer(ASTContext *C, int id, ClangAstDumper dumper)
    : id(id), topLevelDeclVisitor(C, id), printRelationsVisitor(C, id, dumper) {}

// Override the method that gets called for each parsed top-level declaration.
bool MyASTConsumer::HandleTopLevelDecl(DeclGroupRef DR) {

    for (auto *D : DR) {
        try {
            topLevelDeclVisitor.TraverseDecl(D);
        } catch (const std::exception &e) {
            dumpFatalError(D, e.what());
        } catch (...) {
            dumpFatalError(D, "unknown error");
        }
    }

    for (auto *D : DR) {
        try {
            printRelationsVisitor.TraverseDecl(D);
        } catch (const std::exception &e) {
            dumpFatalError(D, e.what());
        } catch (...) {
            dumpFatalError(D, "unknown error");
        }
    }

    return true;
}

void MyASTConsumer::HandleTranslationUnit(ASTContext &Ctx) {}

// For each source file provided to the tool, a new FrontendAction is created.
std::unique_ptr<ASTConsumer>
DumpAstAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    int counter = DumpResources::runId;
    
    // If runId is 0 (default value), use the global counter instead
    if (counter == 0) {
        // TODO: Replace with a global counter. Necessary to enable usage in Clava Node.
    }

    // Register preprocessor callbacks for tracking includes
    // This must be done before AST processing begins
    CI.getPreprocessor().addPPCallbacks(std::make_unique<IncludeDumper>(CI));

    dumpCompilerInstanceData(CI, file);

    // Dump id->file data
    llvm::errs() << ID_FILE_MAP << "\n";
    llvm::errs() << counter << "\n";
    llvm::errs() << file << "\n";

    ASTContext *Context = &CI.getASTContext();

    ClangAstDumper dumper(Context, counter,
                          DumpResources::systemHeaderThreshold);

    return std::make_unique<MyASTConsumer>(Context, counter, dumper);
}

void DumpAstAction::ExecuteAction() {
    ASTFrontendAction::ExecuteAction();
}

void DumpAstAction::dumpCompilerInstanceData(CompilerInstance &CI,
                                             StringRef file) {
    clava::dump(COMPILER_INSTANCE_DATA);

    clava::dump(file.str());

    clava::dump(CI.getInvocation().getLangOpts().LineComment);
    // Derived from Std.isC89 in Clang 3.8
    clava::dump(CI.getInvocation().getLangOpts().GNUInline);
    clava::dump(CI.getInvocation().getLangOpts().C99);
    clava::dump(CI.getInvocation().getLangOpts().C11);
    clava::dump(CI.getInvocation().getLangOpts().CPlusPlus);
    clava::dump(CI.getInvocation().getLangOpts().CPlusPlus11);
    clava::dump(CI.getInvocation().getLangOpts().CPlusPlus14);
    clava::dump(CI.getInvocation().getLangOpts().CPlusPlus17);
    clava::dump(CI.getInvocation().getLangOpts().CPlusPlus20);
    clava::dump(CI.getInvocation().getLangOpts().CPlusPlus23);
    clava::dump(CI.getInvocation().getLangOpts().CPlusPlus26);
    clava::dump(CI.getInvocation().getLangOpts().Digraphs);
    clava::dump(CI.getInvocation().getLangOpts().GNUMode);
    clava::dump(CI.getInvocation().getLangOpts().HexFloats);

    clava::dump(CI.getInvocation().getLangOpts().OpenCL);
    clava::dump(CI.getInvocation().getLangOpts().OpenCLVersion);
    clava::dump(CI.getInvocation().getLangOpts().NativeHalfType);

    clava::dump(CI.getInvocation().getLangOpts().CUDA);

    clava::dump(CI.getInvocation().getLangOpts().Bool);
    clava::dump(CI.getInvocation().getLangOpts().Half);
    clava::dump(CI.getInvocation().getLangOpts().WChar);

    clava::dump(CI.getTarget().getCharWidth());
    clava::dump(CI.getTarget().getFloatWidth());
    clava::dump(CI.getTarget().getDoubleWidth());
    clava::dump(CI.getTarget().getLongDoubleWidth());
    clava::dump(CI.getTarget().getBoolWidth());
    clava::dump(CI.getTarget().getShortWidth());
    clava::dump(CI.getTarget().getIntWidth());
    clava::dump(CI.getTarget().getLongWidth());
    clava::dump(CI.getTarget().getLongLongWidth());
}

/*** IncludeDumper ***/

IncludeDumper::IncludeDumper(CompilerInstance &compilerInstance)
    : compilerInstance(compilerInstance),
      sm(compilerInstance.getSourceManager()){};

void IncludeDumper::InclusionDirective(
    SourceLocation HashLoc, const Token &IncludeTok, StringRef FileName,
    bool IsAngled, CharSourceRange FilenameRange, OptionalFileEntryRef File,
    StringRef SearchPath, StringRef RelativePath, const Module *Imported,
    SrcMgr::CharacteristicKind FileType) {

    if (!sm.isInSystemHeader(HashLoc)) {
        // Includes information in stream
        llvm::errs() << INCLUDES << "\n";
        // Source
        llvm::errs() << sm.getFilename(HashLoc).str() << "\n";
        llvm::errs() << FileName.str() << "\n";
        llvm::errs() << sm.getSpellingLineNumber(HashLoc) << "\n";
        llvm::errs() << IsAngled << "\n";
    }
}

void IncludeDumper::PragmaDirective(SourceLocation Loc,
                                    PragmaIntroducerKind Introducer) {

    // Ignore system headers
    if (sm.isInSystemHeader(Loc)) {
        return;
    }

    // Pragma location
    clava::dump(PRAGMA);
    clava::dump(sm.getFilename(Loc));
    clava::dump(sm.getSpellingLineNumber(Loc));
    clava::dump(sm.getSpellingColumnNumber(Loc));
}

void IncludeDumper::FileChanged(SourceLocation Loc, FileChangeReason Reason,
                                SrcMgr::CharacteristicKind FileType,
                                FileID PrevFID) {}

void IncludeDumper::MacroExpands(const Token &MacroNameTok,
                                 const MacroDefinition &MD, SourceRange Range,
                                 const MacroArgs *Args) {}


/**
 * DumpResources Implementations
 */

// File instantiations
std::ofstream DumpResources::includes;
std::ofstream DumpResources::nodetypes;
int DumpResources::runId;
int DumpResources::systemHeaderThreshold;

void DumpResources::setRunId(int runId) {
    DumpResources::runId = runId;
}

void DumpResources::setSystemHeaderThreshold(int systemHeaderThreshold) {
    DumpResources::systemHeaderThreshold = systemHeaderThreshold;
}

void DumpResources::writeCounter(int id) {

    // Output is processed with a line iterator, allows multiple-line processing

    llvm::outs() << PREFIX << "\n";
    llvm::outs() << id << "\n";

    llvm::errs() << PREFIX << "\n";
    llvm::errs() << id << "\n";
}

void DumpResources::init(int runId, int systemLevelThreshold) {

    DumpResources::runId = runId;
    DumpResources::systemHeaderThreshold = systemLevelThreshold;

    if (HandlerCoverageReport) {
        clava::enableHandlerCoverageReport();
    }
    if (HandlerCoverageAll) {
        clava::enableHandlerEncounterReport();
    }
}

void DumpResources::finish() {
    clava::reportHandlerCoverage();
}
