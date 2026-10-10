//
// Created by JoaoBispo on 20/01/2017.
//

#include "../Clang/ClangNodes.h"
#include "ClangAstDumper.h"

#include "clang/AST/AST.h"

#include <iostream>
#include <sstream>

using namespace clang;


void ClangAstDumper::visitChildrenAndData(const Stmt *S) {
  // Visit children
  visitChildren(S);

  // Dump data
  emit(S);

  // Dump id
  dumpIdToClassMap(S, clava::getClassName(S));
}

void ClangAstDumper::visitChildrenAndData(const Expr *E) {
  // Visit children
  visitChildren(E);

  // Dump data
  emit(E);

  // Dump id
  dumpIdToClassMap(E, clava::getClassName(E));
}

/*
 * STMTS
 */

bool ClangAstDumper::dumpStmt(const Stmt *stmtAddr) {

  if (stmtAddr == nullptr) {
    return true;
  }

  if (seenStmts.count(stmtAddr) != 0) {
    return true;
  }

  // A StmtDumper is created for each context,
  // no need to use id to disambiguate
  seenStmts.insert(stmtAddr);

  return false;
}

void ClangAstDumper::VisitStmt(const Stmt *Node) {
  if (dumpStmt(Node)) {
    return;
  }

  bool isSystemHeader = clava::isSystemHeader(Node, Context);
  if (isSystemHeader) {
    currentSystemHeaderLevel++;
  }

  visitChildrenAndData(Node);

  if (isSystemHeader) {
    currentSystemHeaderLevel--;
  }
}
