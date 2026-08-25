//
// Created by JoaoBispo on 01/02/2017.
//

#ifndef CLANGASTDUMPER_TYPEMARKER_H
#define CLANGASTDUMPER_TYPEMARKER_H

#include "clang/AST/TypeVisitor.h"

#include "llvm/ADT/SmallPtrSet.h"

using namespace clang;

class TypeMarker : public clang::TypeVisitor<TypeMarker> {

  private:
    int id;
    // Seen-type tracking: membership tests/insertions only, never iterated.
    llvm::SmallPtrSet<const Type *, 16> &seenTypes;
    void markType(const Type *T);

  public:
    explicit TypeMarker(int id, llvm::SmallPtrSet<const Type *, 16> &seenTypes);
    void VisitType(const Type *T);
    void VisitTypedefType(const TypedefType *T);
};

#endif // CLANGASTDUMPER_TYPEMARKER_H
