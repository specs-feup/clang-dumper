//
// Created by JoaoBispo on 20/01/2017.
//

#include "../Clang/ClangNodes.h"
#include "../Clava/DumpStream.h"
#include "ClangAstDumper.h"
#include "ClangAstDumperConstants.h"

#include "clang/AST/AST.h"

#include <iostream>
#include <sstream>

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
 * DECLS PARTS
 */
void ClangAstDumper::dumpNumberTemplateParameters(
    const Decl *D, const TemplateParameterList *TPL) {
  int numberOfTemplateParameters = 0;
  if (TPL) {
    for (auto I = TPL->begin(), E = TPL->end(); I != E; ++I) {
      numberOfTemplateParameters++;
    }
  }

  clava::dumpStream() << DUMP_NUMBER_TEMPLATE_PARAMETERS << "\n";
  // Dump id
  clava::dumpStream() << D << "_" << id << "\n";
  // Dump number
  clava::dumpStream() << numberOfTemplateParameters << "\n";
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
