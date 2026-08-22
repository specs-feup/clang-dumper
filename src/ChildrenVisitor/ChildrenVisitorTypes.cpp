//
// Created by JoaoBispo on 05/04/2018.
//
#include "../Clang/ClangNodes.h"
#include "../ClangAstDumper/ClangAstDumper.h"

#include <string>

// Selects the children visitor by class name. Several classes can share one
// visitor (e.g. EnumType and RecordType use the TagType visitor).
#define TYPE_CHILDREN_ENTRY(CLASS, VISITOR)                                    \
  {#CLASS, [](ClangAstDumper &self, const Type *T,                             \
              std::vector<std::string> &children) {                            \
    self.VISITOR(static_cast<const CLASS *>(T), children);                     \
  }}

const std::map<std::string, ClangAstDumper::TypeChildrenFn>
    ClangAstDumper::TYPE_CHILDREN_VISITORS = {
        TYPE_CHILDREN_ENTRY(FunctionProtoType,
                            VisitFunctionProtoTypeChildren),
        TYPE_CHILDREN_ENTRY(FunctionNoProtoType, VisitFunctionTypeChildren),
        TYPE_CHILDREN_ENTRY(ConstantArrayType, VisitArrayTypeChildren),
        TYPE_CHILDREN_ENTRY(DependentSizedArrayType,
                            VisitDependentSizedArrayTypeChildren),
        TYPE_CHILDREN_ENTRY(IncompleteArrayType, VisitArrayTypeChildren),
        TYPE_CHILDREN_ENTRY(VariableArrayType,
                            VisitVariableArrayTypeChildren),
        TYPE_CHILDREN_ENTRY(PointerType, VisitPointerTypeChildren),
        TYPE_CHILDREN_ENTRY(EnumType, VisitTagTypeChildren),
        TYPE_CHILDREN_ENTRY(RecordType, VisitTagTypeChildren),
        TYPE_CHILDREN_ENTRY(ElaboratedType, VisitElaboratedTypeChildren),
        TYPE_CHILDREN_ENTRY(LValueReferenceType,
                            VisitReferenceTypeChildren),
        TYPE_CHILDREN_ENTRY(RValueReferenceType,
                            VisitReferenceTypeChildren),
        TYPE_CHILDREN_ENTRY(InjectedClassNameType,
                            VisitInjectedClassNameTypeChildren),
        TYPE_CHILDREN_ENTRY(TemplateTypeParmType,
                            VisitTemplateTypeParmTypeChildren),
        TYPE_CHILDREN_ENTRY(SubstTemplateTypeParmType,
                            VisitSubstTemplateTypeParmTypeChildren),
        TYPE_CHILDREN_ENTRY(TemplateSpecializationType,
                            VisitTemplateSpecializationTypeChildren),
        TYPE_CHILDREN_ENTRY(TypedefType, VisitTypedefTypeChildren),
        TYPE_CHILDREN_ENTRY(DecayedType, VisitDecayedTypeChildren),
        TYPE_CHILDREN_ENTRY(DecltypeType, VisitDecltypeTypeChildren),
        TYPE_CHILDREN_ENTRY(AutoType, VisitAutoTypeChildren),
        TYPE_CHILDREN_ENTRY(PackExpansionType,
                            VisitPackExpansionTypeChildren),
        TYPE_CHILDREN_ENTRY(TypeOfExprType, VisitTypeOfExprTypeChildren),
        TYPE_CHILDREN_ENTRY(AttributedType, VisitAttributedTypeChildren),
        TYPE_CHILDREN_ENTRY(UnaryTransformType,
                            VisitUnaryTransformTypeChildren),
        TYPE_CHILDREN_ENTRY(ComplexType, VisitComplexTypeChildren),
};

void ClangAstDumper::visitChildren(const Type *T) {
    auto it = TYPE_CHILDREN_VISITORS.find(clava::getClassName(T));

    std::vector<std::string> visitedChildren;
    if (it != TYPE_CHILDREN_VISITORS.end()) {
        it->second(*this, T, visitedChildren);
    } else {
        VisitTypeChildren(T, visitedChildren);
    }

    dumpVisitedChildren(T, visitedChildren);
}

void ClangAstDumper::visitChildren(const QualType &T) {
    std::vector<std::string> visitedChildren;

    VisitTypeTop(T.getTypePtr());

    // Visit unqualified type
    VisitTypeTop(
        T.getSingleStepDesugaredType(*const_cast<const ASTContext *>(Context)));

    dumpVisitedChildren(T.getAsOpaquePtr(), visitedChildren);
}

void ClangAstDumper::VisitTypeChildren(
    const Type *T, std::vector<std::string> &visitedChildren) {

    // If has sugar, visit desugared type
    QualType singleStepDesugar =
        T->getLocallyUnqualifiedSingleStepDesugaredType();

    if (singleStepDesugar != QualType(T, 0)) {
        VisitTypeTop(singleStepDesugar);
    }
}

void ClangAstDumper::VisitFunctionTypeChildren(
    const FunctionType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    // Return type
    VisitTypeTop(T->getReturnType());
}

void ClangAstDumper::VisitFunctionProtoTypeChildren(
    const FunctionProtoType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitFunctionTypeChildren(T, visitedChildren);

    // Parameters types
    for (QualType paramType : T->getParamTypes()) {
        VisitTypeTop(paramType);
    }

    const auto &exptSpec = T->getExtProtoInfo().ExceptionSpec;

    // Visit exception types
    for (auto &exceptType : exptSpec.Exceptions) {
        VisitTypeTop(exceptType);
    }

    // Visit noexcept expression, if present
    VisitStmtTop(exptSpec.NoexceptExpr);

    // Visit source decl, if present
    VisitDeclTop(exptSpec.SourceDecl);

    // Visit source template, if present
    VisitDeclTop(exptSpec.SourceTemplate);
}

void ClangAstDumper::VisitTagTypeChildren(
    const TagType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    // Just visit decl
    VisitDeclTop(T->getDecl());
}

void ClangAstDumper::VisitArrayTypeChildren(
    const ArrayType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    // Element type
    VisitTypeTop(T->getElementType());
}

void ClangAstDumper::VisitVariableArrayTypeChildren(
    const VariableArrayType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitArrayTypeChildren(T, visitedChildren);

    // Visit and add size expression
    VisitStmtTop(T->getSizeExpr());
}

void ClangAstDumper::VisitDependentSizedArrayTypeChildren(
    const DependentSizedArrayType *T,
    std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitArrayTypeChildren(T, visitedChildren);

    // Visit and add size expression
    VisitStmtTop(T->getSizeExpr());
}

void ClangAstDumper::VisitPointerTypeChildren(
    const PointerType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    // Visit pointee
    VisitTypeTop(T->getPointeeType());
}

void ClangAstDumper::VisitElaboratedTypeChildren(
    const ElaboratedType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    // Visit named type
    VisitTypeTop(T->getNamedType());
}

void ClangAstDumper::VisitReferenceTypeChildren(
    const ReferenceType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    VisitTypeTop(T->getPointeeTypeAsWritten());
}

void ClangAstDumper::VisitInjectedClassNameTypeChildren(
    const InjectedClassNameType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    // Visit decl
    VisitDeclTop(T->getDecl());
}

void ClangAstDumper::VisitTemplateTypeParmTypeChildren(
    const TemplateTypeParmType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    // Visit decl
    VisitDeclTop(T->getDecl());
}

void ClangAstDumper::VisitSubstTemplateTypeParmTypeChildren(
    const SubstTemplateTypeParmType *T,
    std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    VisitDeclTop(T->getReplacedParameter());
    VisitTypeTop(T->getReplacementType());
};

void ClangAstDumper::VisitTemplateSpecializationTypeChildren(
    const TemplateSpecializationType *T,
    std::vector<std::string> &visitedChildren) {

    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    // Visit each argument
    for (auto &arg : T->template_arguments()) {
        VisitTemplateArgChildren(arg);
    }

    // Visit type alias
    if (T->isTypeAlias()) {
        VisitTypeTop(T->getAliasedType());
    }

    // Visit template delc, if present
    VisitDeclTop(T->getTemplateName().getAsTemplateDecl());
};

void ClangAstDumper::VisitTypedefTypeChildren(
    const TypedefType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    VisitDeclTop(T->getDecl());
    VisitTypeTop(T->getPointeeType());
};

void ClangAstDumper::VisitAdjustedTypeChildren(
    const AdjustedType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    // Original type
    VisitTypeTop(T->getOriginalType());

    // Adjusted type
    VisitTypeTop(T->getAdjustedType());
};

void ClangAstDumper::VisitDecayedTypeChildren(
    const DecayedType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitAdjustedTypeChildren(T, visitedChildren);

    VisitTypeTop(T->getDecayedType());

    VisitTypeTop(T->getPointeeType());
};

void ClangAstDumper::VisitDecltypeTypeChildren(
    const DecltypeType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    VisitStmtTop(T->getUnderlyingExpr());
};

void ClangAstDumper::VisitAutoTypeChildren(
    const AutoType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    VisitTypeTop(T->getDeducedType());
};

void ClangAstDumper::VisitPackExpansionTypeChildren(
    const PackExpansionType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    if (!T->isSugared()) {
        VisitTypeTop(T->getPattern());
    }
};

void ClangAstDumper::VisitTypeOfExprTypeChildren(
    const TypeOfExprType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    VisitStmtTop(T->getUnderlyingExpr());
};

void ClangAstDumper::VisitAttributedTypeChildren(
    const AttributedType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    VisitTypeTop(T->getModifiedType());
    VisitTypeTop(T->getEquivalentType());
};

void ClangAstDumper::VisitUnaryTransformTypeChildren(
    const UnaryTransformType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    VisitTypeTop(T->getUnderlyingType());
    VisitTypeTop(T->getBaseType());
};

void ClangAstDumper::VisitComplexTypeChildren(
    const ComplexType *T, std::vector<std::string> &visitedChildren) {
    // Hierarchy
    VisitTypeChildren(T, visitedChildren);

    VisitTypeTop(T->getElementType());
};
