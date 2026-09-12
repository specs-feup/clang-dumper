#include "../Clava/FlatStream.h"
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
#include "../Clava/DumpStream.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>

#include <cctype>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <string>

using namespace clang;

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

void dumpFatalError(const Decl *D, const char *message) {
    llvm::errs() << "ERROR "
                 << (D == nullptr ? "<unknown>" : clava::getClassName(D)) << " "
                 << sanitizeErrorMessage(message) << "\n";
    llvm::errs().flush();
    clava::dumpStream().flush();
    clava::reportHandlerCoverage();
    llvm::outs().flush();
}

} // namespace

static constexpr const char *const PREFIX = "COUNTER";

static llvm::cl::opt<bool> HandlerCoverageReport(
    "handler-coverage-report",
    llvm::cl::desc("Report class names that used fallback dump handling"),
    llvm::cl::init(false));

PrintNodesTypesRelationsVisitor::PrintNodesTypesRelationsVisitor(
    ASTContext *Context, int id, int systemHeaderThreshold)
    : Context(Context), dumper(Context, id, systemHeaderThreshold) {}

bool PrintNodesTypesRelationsVisitor::VisitEnumDecl(EnumDecl *D) {
    FullSourceLoc fullLocation = Context->getFullLoc(D->getBeginLoc());
    if (fullLocation.isValid() && !fullLocation.isInSystemHeader()) {
        dumper.VisitTypeTop(D->getIntegerType());
    }

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

MyASTConsumer::MyASTConsumer(ASTContext *C, int id, int systemHeaderThreshold)
    : Context(C), id(id),
      printRelationsVisitor(C, id, systemHeaderThreshold) {}

// Override the method that gets called for each parsed top-level declaration.
bool MyASTConsumer::HandleTopLevelDecl(DeclGroupRef DR) {

    for (auto *D : DR) {
        try {
            if (D == nullptr) {
                continue;
            }

            FullSourceLoc fullLocation = Context->getFullLoc(D->getBeginLoc());
            if (fullLocation.isValid() && fullLocation.hasManager() &&
                !fullLocation.isInSystemHeader()) {
                if(auto *stream=clava::flat::FlatStream::active()) {
                    astwire::v2::TopLevelT record;record.kind=astwire::v2::TopLevelKind::Decl;
                    record.node=clava::flat::wireId(clava::getId(D,id));stream->record(std::move(record));
                } else {
                    clava::dumpStream() << TOP_LEVEL_NODES << "\n";
                    clava::dumpStream() << clava::getId(D,id) << "\n";
                }
            }
        } catch (const std::exception &e) {
            dumpFatalError(D, e.what());
            return false;
        } catch (...) {
            dumpFatalError(D, "unknown error");
            return false;
        }
    }

    for (auto *D : DR) {
        try {
            printRelationsVisitor.TraverseDecl(D);
        } catch (const std::exception &e) {
            dumpFatalError(D, e.what());
            return false;
        } catch (...) {
            dumpFatalError(D, "unknown error");
            return false;
        }
    }

    return true;
}

// For each source file provided to the tool, a new FrontendAction is created.
std::unique_ptr<ASTConsumer>
DumpAstAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    clava::resetDenseIds();
    int counter = DumpResources::runId;
    
    // If runId is 0 (default value), use the global counter instead
    if (counter == 0) {
        // TODO: Replace with a global counter. Necessary to enable usage in Clava Node.
    }

    // Register preprocessor callbacks for tracking includes
    // This must be done before AST processing begins
    CI.getPreprocessor().addPPCallbacks(
        std::make_unique<IncludeDumper>(CI.getSourceManager()));

    dumpCompilerInstanceData(CI, file);

    // Dump id->file data
    if(auto *stream=clava::flat::FlatStream::active()) {
        astwire::v2::TranslationUnitFileT record;record.id=counter;record.path=file.str();stream->record(std::move(record));
    } else {
    clava::dumpStream() << ID_FILE_MAP << "\n";
    clava::dumpStream() << counter << "\n";
    clava::dumpStream() << file << "\n";
    }

    ASTContext *Context = &CI.getASTContext();

    return std::make_unique<MyASTConsumer>(
        Context, counter, DumpResources::systemHeaderThreshold);
}

void DumpAstAction::dumpCompilerInstanceData(CompilerInstance &CI,
                                             StringRef file) {
    if(auto *stream=clava::flat::FlatStream::active()) {
        astwire::v2::LanguageT record;record.file=file.str();
        record.line_comment=+CI.getInvocation().getLangOpts().LineComment;
        record.gnu_inline=+CI.getInvocation().getLangOpts().GNUInline;
        record.c99=+CI.getInvocation().getLangOpts().C99;
        record.c11=+CI.getInvocation().getLangOpts().C11;
        record.c_plus_plus=+CI.getInvocation().getLangOpts().CPlusPlus;
        record.c_plus_plus_11=+CI.getInvocation().getLangOpts().CPlusPlus11;
        record.c_plus_plus_14=+CI.getInvocation().getLangOpts().CPlusPlus14;
        record.c_plus_plus_17=+CI.getInvocation().getLangOpts().CPlusPlus17;
        record.c_plus_plus_20=+CI.getInvocation().getLangOpts().CPlusPlus20;
        record.c_plus_plus_23=+CI.getInvocation().getLangOpts().CPlusPlus23;
        record.c_plus_plus_26=+CI.getInvocation().getLangOpts().CPlusPlus26;
        record.has_digraphs=+CI.getInvocation().getLangOpts().Digraphs;
        record.is_gnu=+CI.getInvocation().getLangOpts().GNUMode;
        record.hex_floats=+CI.getInvocation().getLangOpts().HexFloats;
        record.open_cl=+CI.getInvocation().getLangOpts().OpenCL;
        record.open_cl_version=+CI.getInvocation().getLangOpts().OpenCLVersion;
        record.native_half_type=+CI.getInvocation().getLangOpts().NativeHalfType;
        record.cuda=+CI.getInvocation().getLangOpts().CUDA;
        record.has_bool=+CI.getInvocation().getLangOpts().Bool;
        record.has_half=+CI.getInvocation().getLangOpts().Half;
        record.has_wchar=+CI.getInvocation().getLangOpts().WChar;
        record.char_width=CI.getTarget().getCharWidth();
        record.float_width=CI.getTarget().getFloatWidth();
        record.double_width=CI.getTarget().getDoubleWidth();
        record.long_double_width=CI.getTarget().getLongDoubleWidth();
        record.bool_width=CI.getTarget().getBoolWidth();
        record.short_width=CI.getTarget().getShortWidth();
        record.int_width=CI.getTarget().getIntWidth();
        record.long_width=CI.getTarget().getLongWidth();
        record.long_long_width=CI.getTarget().getLongLongWidth();
        stream->record(std::move(record));return;
    }
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

IncludeDumper::IncludeDumper(const SourceManager &sourceManager)
    : sourceManager(sourceManager) {}

void IncludeDumper::InclusionDirective(
    SourceLocation HashLoc, const Token &IncludeTok, StringRef FileName,
    bool IsAngled, CharSourceRange FilenameRange, OptionalFileEntryRef File,
    StringRef SearchPath, StringRef RelativePath, const Module *Imported,
    SrcMgr::CharacteristicKind FileType) {

    if (!sourceManager.isInSystemHeader(HashLoc)) {
        if(auto *stream=clava::flat::FlatStream::active()) {
            astwire::v2::IncludeT record;record.source=sourceManager.getFilename(HashLoc).str();record.name=FileName.str();
            record.line=sourceManager.getSpellingLineNumber(HashLoc);record.angled=IsAngled;stream->record(std::move(record));return;
        }
        // Includes information in stream
        clava::dumpStream() << INCLUDES << "\n";
        // Source
        clava::dumpStream() << sourceManager.getFilename(HashLoc).str() << "\n";
        clava::dumpStream() << FileName.str() << "\n";
        clava::dumpStream() << sourceManager.getSpellingLineNumber(HashLoc) << "\n";
        clava::dumpStream() << IsAngled << "\n";
    }
}

void IncludeDumper::PragmaDirective(SourceLocation Loc,
                                    PragmaIntroducerKind Introducer) {

    // Ignore system headers
    if (sourceManager.isInSystemHeader(Loc)) {
        return;
    }

    if(auto *stream=clava::flat::FlatStream::active()) {
        astwire::v2::PragmaT record;record.source=sourceManager.getFilename(Loc).str();record.line=sourceManager.getSpellingLineNumber(Loc);
        record.column=sourceManager.getSpellingColumnNumber(Loc);stream->record(std::move(record));return;
    }
    // Pragma location
    clava::dump(PRAGMA);
    clava::dump(sourceManager.getFilename(Loc));
    clava::dump(sourceManager.getSpellingLineNumber(Loc));
    clava::dump(sourceManager.getSpellingColumnNumber(Loc));
}


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
    if(auto *stream=clava::flat::FlatStream::active()) {
        astwire::v2::CounterT record;record.value=id;stream->record(std::move(record));return;
    }

    // Output is processed with a line iterator, allows multiple-line processing

    clava::dumpStream() << PREFIX << "\n";
    clava::dumpStream() << id << "\n";
}

void DumpResources::init(int runId, int systemLevelThreshold) {

    DumpResources::runId = runId;
    DumpResources::systemHeaderThreshold = systemLevelThreshold;

    if (HandlerCoverageReport) {
        clava::enableHandlerCoverageReport();
    }
}

void DumpResources::finish() {
    clava::reportHandlerCoverage();
}
