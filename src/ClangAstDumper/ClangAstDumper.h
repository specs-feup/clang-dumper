//
// Created by JoaoBispo on 20/01/2017.
//

#ifndef CLANGASTDUMPER_CLANGASTDUMPER_H
#define CLANGASTDUMPER_CLANGASTDUMPER_H

#include "../Clava/FlatStream.h"

#include "clang/AST/DeclVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/TypeVisitor.h"

#include "llvm/ADT/SmallPtrSet.h"
#include <string>
#include <vector>

using namespace clang;

class ClangAstDumper : public TypeVisitor<ClangAstDumper>,
                       public ConstStmtVisitor<ClangAstDumper>,
                       public ConstDeclVisitor<ClangAstDumper> {

private:
  ASTContext *Context;
  int id;

  int systemHeaderThreshold = 0; // Overridden by the constructor.
  int currentSystemHeaderLevel = 0;

  // Seen-node tracking: membership tests/insertions only, never iterated.
  llvm::SmallPtrSet<const void *, 32> seenTypes;
  llvm::SmallPtrSet<const Stmt *, 32> seenStmts;
  llvm::SmallPtrSet<const Decl *, 32> seenDecls;
  llvm::SmallPtrSet<const Attr *, 16> seenAttrs;

  /** Writes the node's data record to the dump in progress. */
  template <class T> void emit(const T &node) { clava::flat::FlatStream::current().node(node, Context, id); }

  // Children visitors are selected directly by the node's class name, as
  // reported on the wire format ("<Id to Class Map>" payloads). Adding a
  // handler for a new/renamed Clang class means adding one table entry.
  using DeclChildrenFn = void (*)(ClangAstDumper &, const Decl *,
                                  std::vector<int64_t> &);
  using StmtChildrenFn = void (*)(ClangAstDumper &, const Stmt *,
                                  std::vector<int64_t> &);
  using ExprChildrenFn = void (*)(ClangAstDumper &, const Expr *,
                                  std::vector<int64_t> &);
  using TypeChildrenFn = void (*)(ClangAstDumper &, const Type *,
                                  std::vector<int64_t> &);
  using AttrChildrenFn = void (*)(ClangAstDumper &, const Attr *,
                                  std::vector<int64_t> &);

  static const std::map<std::string, DeclChildrenFn> DECL_CHILDREN_VISITORS;
  static const std::map<std::string, StmtChildrenFn> STMT_CHILDREN_VISITORS;
  static const std::map<std::string, ExprChildrenFn> EXPR_CHILDREN_VISITORS;
  static const std::map<std::string, TypeChildrenFn> TYPE_CHILDREN_VISITORS;
  static const std::map<std::string, AttrChildrenFn> ATTR_CHILDREN_VISITORS;

public:
  explicit ClangAstDumper(ASTContext *Context, int id,
                          int systemHeaderThreashold);

  // Direct visits serialize referenced dependencies and must preserve graph
  // closure. System-header depth is bounded by the addChild() entry points
  // that perform structural descent.
  void VisitTypeTop(const Type *T);
  void VisitTypeTop(const QualType &T);
  void VisitStmtTop(const Stmt *Node);
  void VisitDeclTop(const Decl *Node);
  void VisitAttrTop(const Attr *Node);

  /*
   * TYPES
   */
  void VisitType(const Type *T); // Should not be manually called, instead call
                                 // VisitTypeTop()

  /*
   * STMTS
   */
  void VisitStmt(const Stmt *T); // Should not be manually called, instead call
                                 // VisitStmtTop()

  /*
   * EXPRS
   */
  void VisitExpr(const Expr *Node);

  /*
   * DELCS
   */
  void VisitDecl(const Decl *D); // Should not be manually called, instead call
                                 // VisitDeclTop()

  /*
   * ATTR
   */
  void VisitAttr(const Attr *A);

  /*
   * Utility methods
   */


  /**
   * Adds a child.
   *
   * @param addr
   * @param id
   * @return
   */
  const void addChild(const Decl *addr, std::vector<int64_t> &children);
  const void addChildren(DeclContext::decl_range declRange,
                         std::vector<int64_t> &children);
  const void addChild(const Stmt *addr, std::vector<int64_t> &children);
  const void addChild(const Expr *addr, std::vector<int64_t> &children);
  const void addChild(const Type *addr, std::vector<int64_t> &children);
  const void addChild(const QualType &addr, std::vector<int64_t> &children);
  const void addChild(const Attr *addr, std::vector<int64_t> &children);

  // Private functions
private:

  // Shared implementation for the addChild() overloads; visitTop serializes
  // the node through the appropriate Top-level visit function.
  template <typename T, typename F>
  void addChildInternal(const T *addr, std::vector<int64_t> &children,
                        F &&visitTop);
  template <typename F>
  void addChildInternal(const QualType &addr,
                        std::vector<int64_t> &children, F &&visitTop);

  // Children and data
  void visitChildrenAndData(const Decl *D);
  void visitChildrenAndData(const Stmt *S);
  void visitChildrenAndData(const Expr *E);
  void visitChildrenAndData(const Type *T);
  void visitChildrenAndData(const Attr *A);

  // Children visitors
  void dumpVisitedChildren(const void *pointer,
                           std::vector<int64_t> children);

  void visitChildren(const Decl *D);
  void visitChildren(const Stmt *S);
  void visitChildren(const Expr *E);
  void visitChildren(const Type *T);
  void visitChildren(const Attr *A);
  void visitChildren(const QualType &T);
  void visitTemplateArguments(const TemplateArgumentLoc *templateArgs,
                              unsigned count);
  // A positive N expands through system-header level N and serializes its
  // immediate children as boundary leaves. Non-positive values are unlimited.
  bool isPastSystemHeaderThreshold() const;

  // Children visitors for Decls
  void VisitDeclChildren(const Decl *D, std::vector<int64_t> &children);
  void VisitNamedDeclChildren(const NamedDecl *D,
                              std::vector<int64_t> &children);
  void VisitTypeDeclChildren(const TypeDecl *D,
                             std::vector<int64_t> &children);
  void VisitTagDeclChildren(const TagDecl *D,
                            std::vector<int64_t> &children);
  void VisitEnumDeclChildren(const EnumDecl *D,
                             std::vector<int64_t> &children);
  void VisitRecordDeclChildren(const RecordDecl *D,
                               std::vector<int64_t> &children);
  void VisitCXXRecordDeclChildren(const CXXRecordDecl *D,
                                  std::vector<int64_t> &children);
  void VisitClassTemplateSpecializationDeclChildren(
      const ClassTemplateSpecializationDecl *D,
      std::vector<int64_t> &children);
  void VisitClassTemplatePartialSpecializationDeclChildren(
      const ClassTemplatePartialSpecializationDecl *D,
      std::vector<int64_t> &children);
  void VisitValueDeclChildren(const ValueDecl *D,
                              std::vector<int64_t> &children);
  void VisitFieldDeclChildren(const FieldDecl *D,
                              std::vector<int64_t> &children);
  void VisitFunctionDeclChildren(const FunctionDecl *D,
                                 std::vector<int64_t> &children);
  void VisitCXXMethodDeclChildren(const CXXMethodDecl *D,
                                  std::vector<int64_t> &children);
  void VisitCXXConstructorDeclChildren(const CXXConstructorDecl *D,
                                       std::vector<int64_t> &children);
  void VisitCXXConversionDeclChildren(const CXXConversionDecl *D,
                                      std::vector<int64_t> &children);

  void VisitVarDeclChildren(const VarDecl *D,
                            std::vector<int64_t> &children);
  void VisitParmVarDeclChildren(const ParmVarDecl *D,
                                std::vector<int64_t> &children);

  void VisitTemplateDeclChildren(const TemplateDecl *D,
                                 std::vector<int64_t> &children);
  void
  VisitTemplateTemplateParmDeclChildren(const TemplateTemplateParmDecl *D,
                                        std::vector<int64_t> &children);
  void VisitTemplateTypeParmDeclChildren(const TemplateTypeParmDecl *D,
                                         std::vector<int64_t> &children);
  void VisitEnumConstantDeclChildren(const EnumConstantDecl *D,
                                     std::vector<int64_t> &children);
  void VisitTypedefNameDeclChildren(const TypedefNameDecl *D,
                                    std::vector<int64_t> &children);
  void VisitUsingDirectiveDeclChildren(const UsingDirectiveDecl *D,
                                       std::vector<int64_t> &children);
  void VisitNamespaceDeclChildren(const NamespaceDecl *D,
                                  std::vector<int64_t> &children);
  void VisitFriendDeclChildren(const FriendDecl *D,
                               std::vector<int64_t> &children);
  void VisitNamespaceAliasDeclChildren(const NamespaceAliasDecl *D,
                                       std::vector<int64_t> &children);
  void VisitLinkageSpecDeclChildren(const LinkageSpecDecl *D,
                                    std::vector<int64_t> &children);
  void VisitStaticAssertDeclChildren(const StaticAssertDecl *D,
                                     std::vector<int64_t> &children);
  void VisitNonTypeTemplateParmDeclChildren(const NonTypeTemplateParmDecl *D,
                                            std::vector<int64_t> &children);
  void VisitUsingDeclChildren(const UsingDecl *D,
                              std::vector<int64_t> &children);

  // Children visitors for Stmts
  void VisitStmtChildren(const Stmt *S, std::vector<int64_t> &children);
  void VisitDeclStmtChildren(const DeclStmt *S,
                             std::vector<int64_t> &children);
  void VisitIfStmtChildren(const IfStmt *S, std::vector<int64_t> &children);
  void VisitForStmtChildren(const ForStmt *S,
                            std::vector<int64_t> &children);
  void VisitWhileStmtChildren(const WhileStmt *S,
                              std::vector<int64_t> &children);
  void VisitDoStmtChildren(const DoStmt *S, std::vector<int64_t> &children);
  void VisitCXXForRangeStmtChildren(const CXXForRangeStmt *S,
                                    std::vector<int64_t> &children);
  void VisitCXXCatchStmtChildren(const CXXCatchStmt *S,
                                 std::vector<int64_t> &children);
  void VisitCXXTryStmtChildren(const CXXTryStmt *S,
                               std::vector<int64_t> &children);
  void VisitCaseStmtChildren(const CaseStmt *S,
                             std::vector<int64_t> &children);
  void VisitDefaultStmtChildren(const DefaultStmt *S,
                                std::vector<int64_t> &children);
  void VisitGotoStmtChildren(const GotoStmt *S,
                             std::vector<int64_t> &children);
  void VisitLabelStmtChildren(const LabelStmt *S,
                              std::vector<int64_t> &children);
  void VisitAttributedStmtChildren(const AttributedStmt *S,
                                   std::vector<int64_t> &children);
  void VisitCapturedStmtChildren(const CapturedStmt *S,
                                 std::vector<int64_t> &children);

  // Children visitors for Exprs
  void VisitExprChildren(const Expr *S, std::vector<int64_t> &children);
  void VisitInitListExprChildren(const InitListExpr *E,
                                 std::vector<int64_t> &children);
  void VisitDeclRefExprChildren(const DeclRefExpr *E,
                                std::vector<int64_t> &children);
  void
  VisitDependentScopeDeclRefExprChildren(const DependentScopeDeclRefExpr *E,
                                         std::vector<int64_t> &children);
  void VisitOffsetOfExprChildren(const OffsetOfExpr *E,
                                 std::vector<int64_t> &children);
  void VisitMemberExprChildren(const MemberExpr *E,
                               std::vector<int64_t> &children);
  void
  VisitMaterializeTemporaryExprChildren(const MaterializeTemporaryExpr *E,
                                        std::vector<int64_t> &children);
  void VisitOverloadExprChildren(const OverloadExpr *E,
                                 std::vector<int64_t> &children);
  void VisitCallExprChildren(const CallExpr *E,
                             std::vector<int64_t> &children);
  void VisitCXXMemberCallExprChildren(const CXXMemberCallExpr *E,
                                      std::vector<int64_t> &children);
  void VisitCXXTypeidExprChildren(const CXXTypeidExpr *E,
                                  std::vector<int64_t> &children);
  void VisitExplicitCastExprChildren(const ExplicitCastExpr *E,
                                     std::vector<int64_t> &children);
  void VisitOpaqueValueExprChildren(const OpaqueValueExpr *E,
                                    std::vector<int64_t> &children);
  void
  VisitUnaryExprOrTypeTraitExprChildren(const UnaryExprOrTypeTraitExpr *E,
                                        std::vector<int64_t> &children);
  void VisitCXXNewExprChildren(const CXXNewExpr *E,
                               std::vector<int64_t> &children);
  void VisitCXXDeleteExprChildren(const CXXDeleteExpr *E,
                                  std::vector<int64_t> &children);
  void VisitLambdaExprChildren(const LambdaExpr *E,
                               std::vector<int64_t> &children);
  void VisitSizeOfPackExprChildren(const SizeOfPackExpr *E,
                                   std::vector<int64_t> &children);
  void VisitDesignatedInitExprChildren(const DesignatedInitExpr *E,
                                       std::vector<int64_t> &children);
  void VisitCXXConstructExprChildren(const CXXConstructExpr *E,
                                     std::vector<int64_t> &children);
  void VisitCXXTemporaryObjectExprChildren(const CXXTemporaryObjectExpr *E,
                                           std::vector<int64_t> &children);
  void
  VisitCXXDependentScopeMemberExprChildren(const CXXDependentScopeMemberExpr *E,
                                           std::vector<int64_t> &children);
  void VisitCXXPseudoDestructorExprChildren(const CXXPseudoDestructorExpr *E,
                                            std::vector<int64_t> &children);
  void VisitMSPropertyRefExprChildren(const MSPropertyRefExpr *E,
                                      std::vector<int64_t> &children);

  // Children visitors for Attributes
  void VisitAlignedAttrChildren(const AlignedAttr *A,
                                std::vector<int64_t> &children);
  void VisitTemplateArgument(const TemplateArgument &templateArg);
  void VisitTemplateName(const TemplateName &templateArg);
  // Same as VisitTemplateArgument, but expands argument packs into their
  // elements.
  void VisitTemplateArgChildren(const TemplateArgument &arg);

private:
  // Shared implementation of VisitTemplateArgument/VisitTemplateArgChildren
  void visitTemplateArgument(const TemplateArgument &templateArg,
                             bool expandPacks);

public:

  // Dumpers of other kinds of information
  void dumpIdToClassMap(const void *pointer, std::string className);
  void dumpTopLevelType(const QualType &type);
  void dumpTopLevelAttr(const Attr *attr);

  // Children visitors for Types
  void VisitTypeChildren(const Type *T, std::vector<int64_t> &children);
  void VisitFunctionTypeChildren(const FunctionType *T,
                                 std::vector<int64_t> &visitedChildren);
  void
  VisitFunctionProtoTypeChildren(const FunctionProtoType *T,
                                 std::vector<int64_t> &visitedChildren);
  void VisitTagTypeChildren(const TagType *T,
                            std::vector<int64_t> &visitedChildren);
  void VisitArrayTypeChildren(const ArrayType *T,
                              std::vector<int64_t> &visitedChildren);
  void
  VisitVariableArrayTypeChildren(const VariableArrayType *T,
                                 std::vector<int64_t> &visitedChildren);
  void VisitDependentSizedArrayTypeChildren(
      const DependentSizedArrayType *T,
      std::vector<int64_t> &visitedChildren);
  void VisitPointerTypeChildren(const PointerType *T,
                                std::vector<int64_t> &visitedChildren);
  void VisitMemberPointerTypeChildren(
      const MemberPointerType *T,
      std::vector<int64_t> &visitedChildren);
  void VisitElaboratedTypeChildren(const ElaboratedType *T,
                                   std::vector<int64_t> &visitedChildren);
  void VisitReferenceTypeChildren(const ReferenceType *T,
                                  std::vector<int64_t> &visitedChildren);
  void
  VisitInjectedClassNameTypeChildren(const InjectedClassNameType *T,
                                     std::vector<int64_t> &visitedChildren);
  void
  VisitTemplateTypeParmTypeChildren(const TemplateTypeParmType *T,
                                    std::vector<int64_t> &visitedChildren);
  void VisitSubstTemplateTypeParmTypeChildren(
      const SubstTemplateTypeParmType *T,
      std::vector<int64_t> &visitedChildren);
  void VisitTemplateSpecializationTypeChildren(
      const TemplateSpecializationType *T,
      std::vector<int64_t> &visitedChildren);
  void VisitTypedefTypeChildren(const TypedefType *T,
                                std::vector<int64_t> &visitedChildren);
  void VisitAdjustedTypeChildren(const AdjustedType *T,
                                 std::vector<int64_t> &visitedChildren);
  void VisitDecayedTypeChildren(const DecayedType *T,
                                std::vector<int64_t> &visitedChildren);
  void VisitDecltypeTypeChildren(const DecltypeType *T,
                                 std::vector<int64_t> &visitedChildren);
  void VisitAutoTypeChildren(const AutoType *T,
                             std::vector<int64_t> &visitedChildren);
  void
  VisitPackExpansionTypeChildren(const PackExpansionType *T,
                                 std::vector<int64_t> &visitedChildren);
  void VisitTypeOfExprTypeChildren(const TypeOfExprType *T,
                                   std::vector<int64_t> &visitedChildren);
  void VisitAttributedTypeChildren(const AttributedType *T,
                                   std::vector<int64_t> &visitedChildren);
  void
  VisitUnaryTransformTypeChildren(const UnaryTransformType *T,
                                  std::vector<int64_t> &visitedChildren);
  void VisitComplexTypeChildren(const ComplexType *T,
                                std::vector<int64_t> &visitedChildren);

  // Children visitors for other types of classes
  void VisitNestedNameSpecifierChildren(NestedNameSpecifier *qualifier);

  // These methods return true if the node had been already visited

  bool dumpType(const Type *typeAddr);
  bool dumpType(const QualType &type);
  bool dumpStmt(const Stmt *stmtAddr);
  bool dumpDecl(const Decl *declAddr);
  bool dumpAttr(const Attr *attrAddr);

};

#endif // CLANGASTDUMPER_CLANGASTDUMPER_H
