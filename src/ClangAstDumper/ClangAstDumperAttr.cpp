//
// Created by JoaoBispo on 12/04/2018.
//

#include "../Clang/ClangNodes.h"
#include "ClangAstDumper.h"

#include "clang/AST/Attr.h"

#include <sstream>

using namespace clang;

void ClangAstDumper::VisitAttr(const Attr *A) {
  if (dumpAttr(A)) {
    return;
  }

  visitChildrenAndData(A);
}

bool ClangAstDumper::dumpAttr(const Attr *attrAddr) {
  if (seenAttrs.count(attrAddr) != 0) {
    return true;
  }

  // A Dumper is created for each context,
  // no need to use id to disambiguate
  seenAttrs.insert(attrAddr);

  return false;
}

void ClangAstDumper::visitChildrenAndData(const Attr *A) {
  // Children come first, so the node record can list them.
  emit(A, visitChildren(A));
}
