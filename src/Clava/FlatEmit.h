#pragma once
namespace clang {class Decl;class Stmt;class Expr;class Type;class QualType;class Attr;class ASTContext;}
namespace clava::flat {
bool emit(const clang::Decl *,clang::ASTContext *,int);
bool emit(const clang::Stmt *,clang::ASTContext *,int);
bool emit(const clang::Expr *,clang::ASTContext *,int);
bool emit(const clang::Type *,clang::ASTContext *,int);
bool emit(const clang::Attr *,clang::ASTContext *,int);
bool emit(const clang::QualType &,clang::ASTContext *,int);
}
