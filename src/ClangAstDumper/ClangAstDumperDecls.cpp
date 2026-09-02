//
// Created by JoaoBispo on 20/01/2017.
//

#include "../Clang/ClangNodes.h"
#include "ClangAstDumper.h"
#include "ClangAstDumperConstants.h"

#include "clang/AST/AST.h"

using namespace clang;


void ClangAstDumper::visitChildrenAndData(const Decl *D) {

  // Visit children
  visitChildren(D);

  // Dump data
  dataDumper.dump(D);

  // Dump id
  dumpIdToClassMap(D, clava::getClassName(D));
}

/*
 * DECLS
 */

bool ClangAstDumper::dumpDecl(const Decl *declAddr) {
  if (declAddr == nullptr) {
    return true;
  }

  if (seenDecls.count(declAddr) != 0) {
    return true;
  }

  log(declAddr);

  // A StmtDumper is created for each context,
  // no need to use id to disambiguate
  seenDecls.insert(declAddr);

  return false;
}

void ClangAstDumper::VisitDecl(const Decl *D) {
  if (dumpDecl(D)) {
    return;
  }

  bool isSystemHeader = clava::isSystemHeader(D, Context);
  if (isSystemHeader) {
    currentSystemHeaderLevel++;
  }

  visitChildrenAndData(D);

  if (isSystemHeader) {
    currentSystemHeaderLevel--;
  }
}
