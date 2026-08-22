
//
// Created by JoaoBispo on 18/03/2018.
//

#ifndef CLANGASTDUMPER_CLAVADATADUMPER_H
#define CLANGASTDUMPER_CLAVADATADUMPER_H

#include "../Clava/ClavaConstants.h"

#include "clang/AST/AST.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Attrs.inc"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"

#include <map>
#include <string>

using namespace clang;

namespace clava {

/**
 * Dumps information about each node. Data dumper selection is table-driven:
 * each table maps a Clang class name (as reported on the wire) to the data
 * section name and the dump function for that class. Adding support for a
 * new/renamed Clang class means adding one table entry.
 */
class ClavaDataDumper {

  private:
    ASTContext *Context;
    const int id;

    struct DeclDataEntry {
        const char *dataName;
        void (*dump)(ClavaDataDumper &, const Decl *);
    };
    struct StmtDataEntry {
        const char *dataName;
        void (*dump)(ClavaDataDumper &, const Stmt *);
    };
    struct ExprDataEntry {
        const char *dataName;
        void (*dump)(ClavaDataDumper &, const Expr *);
    };
    struct TypeDataEntry {
        const char *dataName;
        void (*dump)(ClavaDataDumper &, const Type *);
    };
    struct AttrDataEntry {
        const char *dataName;
        void (*dump)(ClavaDataDumper &, const Attr *);
    };

    static const std::map<std::string, DeclDataEntry> DECL_DATA_DUMPERS;
    static const std::map<std::string, StmtDataEntry> STMT_DATA_DUMPERS;
    static const std::map<std::string, ExprDataEntry> EXPR_DATA_DUMPERS;
    static const std::map<std::string, TypeDataEntry> TYPE_DATA_DUMPERS;
    static const std::map<std::string, AttrDataEntry> ATTR_DATA_DUMPERS;

  public:
    // Constructor
    explicit ClavaDataDumper(ASTContext *Context, int id);

    // Utility methods
    void dump(const Decl *D);
    void dump(const Stmt *S);
    void dump(const Expr *E);
    void dump(const Type *T);
    void dump(const QualType &T);
    void dump(const Attr *A);

  private:
    // DECLS
    void DumpDeclData(const Decl *D);
    void DumpNamedDeclData(const NamedDecl *D);
    void DumpTypeDeclData(const TypeDecl *D);
    void
    DumpUnresolvedUsingTypenameDeclData(const UnresolvedUsingTypenameDecl *D);
    void DumpTagDeclData(const TagDecl *D);
    void DumpEnumDeclData(const EnumDecl *D);
    void DumpRecordDeclData(const RecordDecl *D);
    void DumpCXXRecordDeclData(const CXXRecordDecl *D);
    void DumpClassTemplateSpecializationDeclData(
        const ClassTemplateSpecializationDecl *D);
    void DumpClassTemplatePartialSpecializationDeclData(
        const ClassTemplatePartialSpecializationDecl *D);
    void DumpValueDeclData(const ValueDecl *D);
    void DumpDeclaratorDeclData(const DeclaratorDecl *D);
    void DumpFieldDeclData(const FieldDecl *D);
    void DumpFunctionDeclData(const FunctionDecl *D);
    void DumpCXXMethodDeclData(const CXXMethodDecl *D);
    void DumpCXXConstructorDeclData(const CXXConstructorDecl *D);
    void DumpCXXConversionDeclData(const CXXConversionDecl *D);
    void DumpVarDeclData(const VarDecl *D);
    void DumpParmVarDeclData(const ParmVarDecl *D);
    void DumpTemplateTypeParmDeclData(const TemplateTypeParmDecl *D);
    void DumpTypedefNameDeclData(const TypedefNameDecl *D);
    void DumpAccessSpecDeclData(const AccessSpecDecl *D);
    void DumpUsingDeclData(const UsingDecl *D);
    void DumpUsingDirectiveDeclData(const UsingDirectiveDecl *D);
    void DumpNamespaceDeclData(const NamespaceDecl *D);
    void DumpNamespaceAliasDeclData(const NamespaceAliasDecl *D);
    void DumpLinkageSpecDeclData(const LinkageSpecDecl *D);
    void DumpStaticAssertDeclData(const StaticAssertDecl *D);
    void DumpTemplateTemplateParmDeclData(const TemplateTemplateParmDecl *D);
    void DumpNonTypeTemplateParmDeclData(const NonTypeTemplateParmDecl *D);
    void DumpTemplateDeclData(const TemplateDecl *D);
    void DumpMSPropertyDeclData(const MSPropertyDecl *D);

    // STMTS
    void DumpStmtData(const Stmt *S);
    void DumpLabelStmtData(const LabelStmt *S);
    void DumpGotoStmtData(const GotoStmt *S);
    void DumpAttributedStmtData(const AttributedStmt *S);
    void DumpAsmStmtData(const AsmStmt *S);
    void DumpGCCAsmStmtData(const GCCAsmStmt *S);
    void DumpMSAsmStmtData(const MSAsmStmt *S);

    // EXPRS
    void DumpExprData(const Expr *E);
    void DumpCastExprData(const CastExpr *E);
    void DumpLiteralData(const Expr *E);
    void DumpCharacterLiteralData(const CharacterLiteral *E);
    void DumpIntegerLiteralData(const IntegerLiteral *E);
    void DumpFloatingLiteralData(const FloatingLiteral *E);
    void DumpStringLiteralData(const StringLiteral *E);
    void DumpCXXBoolLiteralExprData(const CXXBoolLiteralExpr *E);
    void DumpCompoundLiteralExprData(const CompoundLiteralExpr *E);
    void DumpInitListExprData(const InitListExpr *E);
    void DumpDeclRefExprData(const DeclRefExpr *E);
    void DumpDependentScopeDeclRefExprData(const DependentScopeDeclRefExpr *E);
    void DumpOverloadExprData(const OverloadExpr *E);
    void DumpUnresolvedMemberExprData(const UnresolvedMemberExpr *E);
    void DumpUnresolvedLookupExprData(const UnresolvedLookupExpr *E);
    void DumpCXXConstructExprData(const CXXConstructExpr *E);
    void DumpCXXTemporaryObjectExprData(const CXXTemporaryObjectExpr *E);
    void DumpMemberExprData(const MemberExpr *E);
    void DumpMaterializeTemporaryExprData(const MaterializeTemporaryExpr *E);
    void DumpBinaryOperatorData(const BinaryOperator *E);
    void DumpCallExprData(const CallExpr *E);
    void DumpCXXMemberCallExprData(const CXXMemberCallExpr *E);
    void DumpCXXTypeidExprData(const CXXTypeidExpr *E);
    void DumpExplicitCastExprData(const ExplicitCastExpr *E);
    void DumpCXXNamedCastExprData(const CXXNamedCastExpr *E);
    void
    DumpCXXDependentScopeMemberExprData(const CXXDependentScopeMemberExpr *E);
    void DumpUnaryOperatorData(const UnaryOperator *E);
    void DumpUnaryExprOrTypeTraitExprData(const UnaryExprOrTypeTraitExpr *E);
    void DumpCXXNewExprData(const CXXNewExpr *E);
    void DumpCXXDeleteExprData(const CXXDeleteExpr *E);
    void DumpOffsetOfExprData(const OffsetOfExpr *E);
    void DumpLambdaExprData(const LambdaExpr *E);
    void DumpPredefinedExprData(const PredefinedExpr *E);
    void DumpSizeOfPackExprData(const SizeOfPackExpr *E);
    void DumpArrayInitLoopExprData(const ArrayInitLoopExpr *E);
    void DumpDesignatedInitExprData(const DesignatedInitExpr *E);
    void DumpCXXNoexceptExprData(const CXXNoexceptExpr *E);
    void DumpCXXPseudoDestructorExprData(const CXXPseudoDestructorExpr *E);
    void DumpPseudoObjectExprData(const PseudoObjectExpr *E);
    void DumpMSPropertyRefExprData(const MSPropertyRefExpr *E);

    // TYPES
    void DumpTypeData(const Type *T);
    void DumpTypeData(const Type *T, Qualifiers &qualifiers);
    void DumpBuiltinTypeData(const BuiltinType *T);
    void DumpPointerTypeData(const PointerType *T);
    void DumpFunctionTypeData(const FunctionType *T);
    void DumpFunctionProtoTypeData(const FunctionProtoType *T);
    void DumpTagTypeData(const TagType *T);
    void DumpArrayTypeData(const ArrayType *T);
    void DumpConstantArrayTypeData(const ConstantArrayType *T);
    void DumpVariableArrayTypeData(const VariableArrayType *T);
    void DumpDependentSizedArrayTypeData(const DependentSizedArrayType *T);
    void DumpTypeWithKeywordData(const TypeWithKeyword *T);
    void DumpElaboratedTypeData(const ElaboratedType *T);
    void DumpTemplateTypeParmTypeData(const TemplateTypeParmType *T);
    void
    DumpTemplateSpecializationTypeData(const TemplateSpecializationType *T);
    void DumpTypedefTypeData(const TypedefType *T);
    void DumpAdjustedTypeData(const AdjustedType *T);
    void DumpDecayedTypeData(const DecayedType *T);
    void DumpDecltypeTypeData(const DecltypeType *T);
    void DumpAutoTypeData(const AutoType *T);
    void DumpReferenceTypeData(const ReferenceType *T);
    void DumpPackExpansionTypeData(const PackExpansionType *T);
    void DumpTypeOfExprTypeData(const TypeOfExprType *T);
    void DumpAttributedTypeData(const AttributedType *T);
    void DumpUnaryTransformTypeData(const UnaryTransformType *T);
    void DumpSubstTemplateTypeParmTypeData(const SubstTemplateTypeParmType *T);
    void DumpComplexTypeData(const ComplexType *T);

    // ATTRS
    void DumpAttrData(const Attr *A);
    void DumpAlignedAttrData(const AlignedAttr *A);
    void DumpOpenCLUnrollHintAttrData(const OpenCLUnrollHintAttr *A);
    void DumpFormatAttrData(const FormatAttr *A);
    void DumpNonNullAttrData(const NonNullAttr *A);
    void DumpVisibilityAttrData(const VisibilityAttr *A);
};

} // namespace clava

#endif // CLANGASTDUMPER_CLAVADATADUMPER_H
