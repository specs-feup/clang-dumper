//
// Created by JoaoBispo on 20/03/2018.
//

#include "../Clang/ClangNodes.h"
#include "../ClangAstDumper/ClangAstDumper.h"
#include "../Clava/ClavaConstants.h"
#include "../Clava/HandlerCoverage.h"

#include <string>

// Selects the children visitor by class name. Entries whose visitor does not
// match their own class use DECL_CHILDREN_ENTRY_AS with the visitor's section.
#define DECL_CHILDREN_ENTRY(CLASS, VISITOR)                                    \
  {#CLASS, [](ClangAstDumper &self, const Decl *D,                             \
              std::vector<std::string> &children) {                            \
    self.VISITOR(static_cast<const CLASS *>(D), children);                     \
  }}

const std::map<std::string, ClangAstDumper::DeclChildrenFn>
    ClangAstDumper::DECL_CHILDREN_VISITORS = {
        DECL_CHILDREN_ENTRY(CXXConstructorDecl, VisitCXXConstructorDeclChildren),
        DECL_CHILDREN_ENTRY(CXXConversionDecl, VisitCXXConversionDeclChildren),
        DECL_CHILDREN_ENTRY(CXXDestructorDecl, VisitCXXMethodDeclChildren),
        DECL_CHILDREN_ENTRY(CXXMethodDecl, VisitCXXMethodDeclChildren),
        DECL_CHILDREN_ENTRY(EnumDecl, VisitEnumDeclChildren),
        DECL_CHILDREN_ENTRY(RecordDecl, VisitRecordDeclChildren),
        DECL_CHILDREN_ENTRY(CXXRecordDecl, VisitCXXRecordDeclChildren),
        DECL_CHILDREN_ENTRY(ClassTemplateSpecializationDecl,
                            VisitClassTemplateSpecializationDeclChildren),
        DECL_CHILDREN_ENTRY(ClassTemplatePartialSpecializationDecl,
                            VisitClassTemplateSpecializationDeclChildren),
        DECL_CHILDREN_ENTRY(FunctionDecl, VisitFunctionDeclChildren),
        DECL_CHILDREN_ENTRY(VarDecl, VisitVarDeclChildren),
        DECL_CHILDREN_ENTRY(ParmVarDecl, VisitVarDeclChildren),
        DECL_CHILDREN_ENTRY(TypeDecl, VisitTypeDeclChildren),
        DECL_CHILDREN_ENTRY(FieldDecl, VisitFieldDeclChildren),
        DECL_CHILDREN_ENTRY(ClassTemplateDecl, VisitTemplateDeclChildren),
        DECL_CHILDREN_ENTRY(FunctionTemplateDecl, VisitTemplateDeclChildren),
        DECL_CHILDREN_ENTRY(TypeAliasTemplateDecl, VisitTemplateDeclChildren),
        DECL_CHILDREN_ENTRY(VarTemplateDecl, VisitTemplateDeclChildren),
        DECL_CHILDREN_ENTRY(TemplateTemplateParmDecl,
                            VisitTemplateTemplateParmDeclChildren),
        DECL_CHILDREN_ENTRY(TemplateTypeParmDecl,
                            VisitTemplateTypeParmDeclChildren),
        DECL_CHILDREN_ENTRY(EnumConstantDecl, VisitEnumConstantDeclChildren),
        DECL_CHILDREN_ENTRY(TypeAliasDecl, VisitTypedefNameDeclChildren),
        DECL_CHILDREN_ENTRY(TypedefDecl, VisitTypedefNameDeclChildren),
        DECL_CHILDREN_ENTRY(UsingDirectiveDecl,
                            VisitUsingDirectiveDeclChildren),
        DECL_CHILDREN_ENTRY(NamespaceDecl, VisitNamespaceDeclChildren),
        DECL_CHILDREN_ENTRY(FriendDecl, VisitFriendDeclChildren),
        DECL_CHILDREN_ENTRY(NamespaceAliasDecl,
                            VisitNamespaceAliasDeclChildren),
        DECL_CHILDREN_ENTRY(LinkageSpecDecl, VisitLinkageSpecDeclChildren),
        DECL_CHILDREN_ENTRY(StaticAssertDecl, VisitStaticAssertDeclChildren),
        DECL_CHILDREN_ENTRY(NonTypeTemplateParmDecl,
                            VisitNonTypeTemplateParmDeclChildren),
        // TODO: Check if needs more data to dump
        DECL_CHILDREN_ENTRY(VarTemplateSpecializationDecl,
                            VisitVarDeclChildren),
        DECL_CHILDREN_ENTRY(UsingDecl, VisitUsingDeclChildren),
};

void ClangAstDumper::visitChildren(const Decl *D) {
    const std::string classname = clava::getClassName(D);
    auto it = DECL_CHILDREN_VISITORS.find(classname);

    std::vector<std::string> visitedChildren;
    if (it != DECL_CHILDREN_VISITORS.end()) {
        it->second(*this, D, visitedChildren);
    } else {
        clava::recordHandlerFallback("decl children", classname);
        VisitDeclChildren(D, visitedChildren);
    }

    dumpVisitedChildren(D, visitedChildren);
}

void ClangAstDumper::VisitDeclChildren(const Decl *D,
                                       std::vector<std::string> &children) {
    // Visit attributes
    for (Decl::attr_iterator I = D->attr_begin(), E = D->attr_end(); I != E;
         ++I) {
        const Attr *attr = *I;
        VisitAttrTop(attr);
        dumpTopLevelAttr(attr);
    }
}

void ClangAstDumper::VisitNamedDeclChildren(
    const NamedDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitDeclChildren(D, children);
}

void ClangAstDumper::VisitTypeDeclChildren(const TypeDecl *D,
                                           std::vector<std::string> &children) {
    // Hierarchy
    VisitNamedDeclChildren(D, children);

    // Visit type
    VisitTypeTop(D->getTypeForDecl());
    dumpType(D->getTypeForDecl());
}

void ClangAstDumper::VisitTagDeclChildren(const TagDecl *D,
                                          std::vector<std::string> &children) {
    // Hierarchy
    VisitTypeDeclChildren(D, children);

    addChildren(D->decls(), children);
}

void ClangAstDumper::VisitEnumDeclChildren(const EnumDecl *D,
                                           std::vector<std::string> &children) {
    // Hierarchy
    VisitTagDeclChildren(D, children);

    // Visit type
    VisitTypeTop(D->getIntegerType());
}

void ClangAstDumper::VisitValueDeclChildren(
    const ValueDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitNamedDeclChildren(D, children);

    // Visit type
    VisitTypeTop(D->getType());
    dumpTopLevelType(D->getType());
}
void ClangAstDumper::VisitFieldDeclChildren(
    const FieldDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitValueDeclChildren(D, children);

    // Add bitwidth
    addChild(D->getBitWidth(), children);

    // Add init
    addChild(D->getInClassInitializer(), children);
}

void ClangAstDumper::VisitFunctionDeclChildren(
    const FunctionDecl *D, std::vector<std::string> &children) {

    // Hierarchy
    VisitValueDeclChildren(D, children);

    // Visit canonical and previous decls
    VisitDeclTop(D->getPreviousDecl());
    VisitDeclTop(D->getCanonicalDecl());

    if (FunctionTemplateDecl const *primaryTemplate = D->getPrimaryTemplate();
        primaryTemplate != nullptr) {
        VisitDeclTop(primaryTemplate->getTemplatedDecl());
    }

    // Visit template arguments
    if (auto templateSpecializationArgs = D->getTemplateSpecializationArgs();
        templateSpecializationArgs != nullptr) {
        for (auto const &templateArg : templateSpecializationArgs->asArray()) {
            VisitTemplateArgument(templateArg);
        }
    }

    // Visit parameters
    for (auto param : D->parameters()) {
        addChild(param, children);
    }

    // Visit body
    if (D->doesThisDeclarationHaveABody()) {
        addChild(D->getBody(), children);
    }
}

void ClangAstDumper::VisitCXXMethodDeclChildren(
    const CXXMethodDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitFunctionDeclChildren(D, children);

    // Visit record decl
    VisitDeclTop(D->getParent());

    // Visit overridden methods
    for (auto overriddenMethod : D->overridden_methods()) {
        VisitDeclTop(overriddenMethod);
    }

    // Types related to "this"
    if (D->isInstance()) {
        VisitTypeTop(D->getThisType());
        VisitTypeTop(D->getFunctionObjectParameterType());
    }
}

void ClangAstDumper::VisitCXXConstructorDeclChildren(
    const CXXConstructorDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitCXXMethodDeclChildren(D, children);

    // Visit CXXCtorInitializers
    for (auto init = D->init_begin(), init_last = D->init_end();
         init != init_last; ++init) {
        // Init expr
        VisitStmtTop((*init)->getInit());

        if ((*init)->isAnyMemberInitializer()) {
            VisitDeclTop((*init)->getAnyMember());
            continue;
        }

        if ((*init)->isBaseInitializer()) {
            VisitTypeTop((*init)->getBaseClass());
            continue;
        }

        if ((*init)->isDelegatingInitializer()) {
            VisitTypeTop((*init)->getTypeSourceInfo()->getType());
            continue;
        }

        throw std::invalid_argument(
            "ClangDataDumper::VisitCXXConstructorDeclChildren():: "
            "CXXCtorInitializer case not implemented");
    }

    // Visit ExplicitSpecifier Expression
    VisitExpr(D->getExplicitSpecifier().getExpr());
}

void ClangAstDumper::VisitCXXConversionDeclChildren(
    const CXXConversionDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitCXXMethodDeclChildren(D, children);

    // Visit fields
    VisitTypeTop(D->getConversionType());
}

void ClangAstDumper::VisitRecordDeclChildren(
    const RecordDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitTagDeclChildren(D, children);
}

void ClangAstDumper::VisitCXXRecordDeclChildren(
    const CXXRecordDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitRecordDeclChildren(D, children);

    if (D->hasDefinition()) {
        // Visit types in bases
        for (const auto &I : D->bases()) {
            VisitTypeTop(I.getType());
        }
        VisitDeclTop(D->getDefinition());
    }
}
void ClangAstDumper::VisitClassTemplateSpecializationDeclChildren(
    const ClassTemplateSpecializationDecl *D,
    std::vector<std::string> &children) {
    // Hierarchy
    VisitCXXRecordDeclChildren(D, children);

    VisitDeclTop(D->getSpecializedTemplate());

    // Visit template arguments
    auto &templateArgs = D->getTemplateArgs();
    for (auto &templateArg : templateArgs.asArray()) {
        VisitTemplateArgument(templateArg);
    }
}

void ClangAstDumper::VisitVarDeclChildren(const VarDecl *D,
                                          std::vector<std::string> &children) {
    // Hierarchy
    VisitValueDeclChildren(D, children);

    if (D->hasInit()) {
        addChild(D->getInit(), children);
    }
}

void ClangAstDumper::VisitTemplateDeclChildren(
    const TemplateDecl *D, std::vector<std::string> &children) {

    // Hierarchy
    VisitNamedDeclChildren(D, children);

    if (auto templateParams = D->getTemplateParameters()) {
        for (auto I = templateParams->begin(), E = templateParams->end();
             I != E; ++I) {
            addChild(*I, children);
            VisitDeclTop(*I);
        }
    }

    addChild(D->getTemplatedDecl(), children);
    VisitDeclTop(D->getTemplatedDecl());
}

void ClangAstDumper::VisitTemplateTemplateParmDeclChildren(
    const TemplateTemplateParmDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitTemplateDeclChildren(D, children);

    if (D->hasDefaultArgument()) {
        VisitTemplateArgument(D->getDefaultArgument().getArgument());
    }
}

void ClangAstDumper::VisitTemplateTypeParmDeclChildren(
    const TemplateTypeParmDecl *D, std::vector<std::string> &children) {

    // Hierarchy
    VisitTypeDeclChildren(D, children);

    if (D->hasDefaultArgument()) {
        VisitTypeTop(D->getDefaultArgument());
    }
}

void ClangAstDumper::VisitEnumConstantDeclChildren(
    const EnumConstantDecl *D, std::vector<std::string> &children) {

    // Hierarchy
    VisitValueDeclChildren(D, children);

    addChild(D->getInitExpr(), children);
}

void ClangAstDumper::VisitTypedefNameDeclChildren(
    const TypedefNameDecl *D, std::vector<std::string> &children) {

    // Hierarchy
    VisitTypeDeclChildren(D, children);

    VisitTypeTop(D->getUnderlyingType());
}

void ClangAstDumper::VisitUsingDirectiveDeclChildren(
    const UsingDirectiveDecl *D, std::vector<std::string> &children) {

    // Hierarchy
    VisitNamedDeclChildren(D, children);

    VisitDeclTop(D->getNominatedNamespace());
    VisitDeclTop(D->getNominatedNamespaceAsWritten());
}

void ClangAstDumper::VisitNamespaceDeclChildren(
    const NamespaceDecl *D, std::vector<std::string> &children) {

    // Hierarchy
    VisitNamedDeclChildren(D, children);

    addChildren(D->decls(), children);
}

void ClangAstDumper::VisitFriendDeclChildren(
    const FriendDecl *D, std::vector<std::string> &children) {

    // Hierarchy
    VisitDeclChildren(D, children);

    if (D->getFriendDecl() != nullptr) {
        addChild(D->getFriendDecl(), children);
    } else if (D->getFriendType() != nullptr) {
        addChild(D->getFriendType()->getType(), children);
    } else {
        // Add a null node
        addChild((const Decl *)nullptr, children);
    }
}

void ClangAstDumper::VisitNamespaceAliasDeclChildren(
    const NamespaceAliasDecl *D, std::vector<std::string> &children) {

    // Hierarchy
    VisitNamedDeclChildren(D, children);

    VisitDeclTop(D->getAliasedNamespace());
}

void ClangAstDumper::VisitLinkageSpecDeclChildren(
    const LinkageSpecDecl *D, std::vector<std::string> &children) {

    addChildren(D->decls(), children);
}

void ClangAstDumper::VisitStaticAssertDeclChildren(
    const StaticAssertDecl *D, std::vector<std::string> &children) {

    addChild(D->getAssertExpr(), children);
    addChild(D->getMessage(), children);
}

void ClangAstDumper::VisitNonTypeTemplateParmDeclChildren(
    const NonTypeTemplateParmDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitValueDeclChildren(D, children);

    if (D->hasDefaultArgument()) {
        VisitStmtTop(D->getDefaultArgument());
    }

    if (D->isExpandedParameterPack()) {
        for (unsigned int i = 0; i < D->getNumExpansionTypes(); i++) {
            VisitTypeTop(D->getExpansionType(i));
        }
    }
}

void ClangAstDumper::VisitUsingDeclChildren(
    const UsingDecl *D, std::vector<std::string> &children) {
    // Hierarchy
    VisitNamedDeclChildren(D, children);

    ClangAstDumper::VisitNestedNameSpecifierChildren(D->getQualifier());
}