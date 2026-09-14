//
// Created by JoaoBispo on 21/03/2018.
//

#ifndef CLANGASTDUMPER_CLANGNODES_H
#define CLANGASTDUMPER_CLANGNODES_H

#include "clang/AST/AST.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/NestedNameSpecifier.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/TemplateName.h"
#include "clang/AST/Type.h"

#include <string>
#include <type_traits>
#include <vector>

using namespace clang;

namespace clava {
/**
 * Returns the name of the class of the given Decl.
 * @param D
 * @return
 */
const std::string getClassName(const Decl *D);

/**
 * Returns the name of the class of the given Stmt.
 * @param S
 * @return
 */
const std::string getClassName(const Stmt *S);

/**
 * Returns the name of the class of the given Type.
 *
 * @param T
 * @return
 */
const std::string getClassName(const Type *T);

/**
 * Returns the kind name of the class of the given Attribute.
 *
 * @param A
 * @return
 */
const std::string getAttrKind(const Attr *A);

/**
 * Returns the name of the class of the given Attribute.
 *
 * @param A
 * @return
 */
const std::string getClassName(const Attr *A);

/**
 * Builds a string id.
 *
 * @param addr
 * @param id
 * @return
 */
const std::string getId(const Decl *addr, int id);
const std::string getId(const Stmt *addr, int id);
const std::string getId(const Expr *addr, int id);
const std::string getId(std::optional<const Expr *> addr, int id);
const std::string getId(const Type *addr, int id);
const std::string getId(const QualType &addr, int id);
const std::string getId(const Attr *addr, int id);

/**
 * Should only be used internally by functions of this include.
 * @param addr
 * @param id
 * @return
 */
const std::string getId(const void *addr, int id);
void enableDenseIds();
void resetDenseIds();
size_t denseIdCount();

void throwNotImplemented(const std::string &source,
                         const std::string &caseNotImplemented,
                         ASTContext *Context, SourceLocation startLoc,
                         SourceLocation endLoc);
void throwNotImplemented(const std::string &source,
                         const std::string &caseNotImplemented,
                         ASTContext *Context, SourceRange range);

bool isSystemHeader(const Stmt *S, ASTContext *context);
bool isSystemHeader(const Decl *S, ASTContext *context);

const std::string getQualifiedPrefix(const NamedDecl *D);

} // namespace clava

#endif // CLANGASTDUMPER_CLANGNODES_H
