//
// Created by JoaoBispo on 01/05/2016.
//
// Based on public domain code by Eli Bendersky (eliben@gmail.com)
// http://eli.thegreenplace.net/
//

#ifndef CLANGASTDUMPER_CLANGAST_H
#define CLANGASTDUMPER_CLANGAST_H

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendActions.h>

#include "../ClangAstDumper/ClangAstDumper.h"

using namespace clang;

// Class for managing ClavaDump resources
class DumpResources {

  public:
    static void init(int runId, int systemLevelThreshold);
    static void finish();

    static void writeCounter(int set);

    static std::ofstream includes;
    static std::ofstream nodetypes;
    static int runId;
    static int systemHeaderThreshold;

    static void setRunId(int runId);
    static void setSystemHeaderThreshold(int systemHeaderThreshold);

  private:
};

/**
 * Dumps includes to a file.
 *
 * Based on explanation from this website:
 * https://xaizek.github.io/2015-04-23/detecting-wrong-first-include/
 */
class IncludeDumper : public PPCallbacks {
  public:
    explicit IncludeDumper(const SourceManager &sourceManager);

    virtual void InclusionDirective(
        SourceLocation HashLoc, const Token &IncludeTok, StringRef FileName,
        bool IsAngled, CharSourceRange FilenameRange, OptionalFileEntryRef File,
        StringRef SearchPath, StringRef RelativePath, const Module *Imported,
        SrcMgr::CharacteristicKind FileType) override;
    virtual void PragmaDirective(SourceLocation Loc,
                                 PragmaIntroducerKind Introducer) override;

  private:
    const SourceManager &sourceManager;
};

// For each source file provided to the tool, a new FrontendAction is created.
class DumpAstAction : public ASTFrontendAction {
  public:
    virtual std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &CI, StringRef file) override;

    void dumpCompilerInstanceData(CompilerInstance &CI, StringRef file);
};

class PrintNodesTypesRelationsVisitor
    : public RecursiveASTVisitor<PrintNodesTypesRelationsVisitor> {

  private:
    ASTContext *Context;
    ClangAstDumper dumper;

  public:
    explicit PrintNodesTypesRelationsVisitor(ASTContext *Context, int id,
                                             int systemHeaderThreshold);
    bool VisitEnumDecl(EnumDecl *D);
    bool VisitDecl(Decl *D);
    bool VisitStmt(Stmt *D);
};

// Implementation of the ASTConsumer interface for reading an AST produced by
// the Clang parser.
class MyASTConsumer : public ASTConsumer {

  private:
    ASTContext *Context;
    int id;
    PrintNodesTypesRelationsVisitor printRelationsVisitor;

  public:
    MyASTConsumer(ASTContext *C, int id, int systemHeaderThreshold);

    bool HandleTopLevelDecl(DeclGroupRef DR) override;
};
#endif // CLANGASTDUMPER_CLANGAST_H
