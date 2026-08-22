//
// Created by JoaoBispo on 18/03/2018.
//

#include "ClavaDataDumper.h"

#include "../Clang/ClangNodes.h"

clava::ClavaDataDumper::ClavaDataDumper(ASTContext *Context, int id)
    : Context(Context), id(id){};
