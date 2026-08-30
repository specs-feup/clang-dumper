//
// Created by JoaoBispo on 20/01/2017.
//

#include "ClangAstDumper.h"
#include "../Clava/DumpStream.h"

#include "clang/AST/AST.h"

using namespace clang;


/*
 * EXTRA Nodes
 */

void ClangAstDumper::dumpCXXCtorInitializer(const CXXCtorInitializer *Init) {

    if (Init->isAnyMemberInitializer()) {
        clava::dumpStream() << "AnyMemberInitializer\n";
    } else if (Init->isBaseInitializer()) {
        // QUALTYPE EXP
        clava::dumpStream() << "BaseInitializer:" << QualType(Init->getBaseClass(), 0).getAsOpaquePtr() << "_" << id << "\n";
    } else if (Init->isDelegatingInitializer()) {
        clava::dumpStream() << "DelegatingInitializer:" << Init->getTypeSourceInfo()->getType().getTypePtr() << "_" << id << "\n";
    } else {
        llvm_unreachable("ClangAstDumper: Unknown initializer type");
    }

}
