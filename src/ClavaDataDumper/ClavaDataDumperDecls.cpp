//
// Created by JoaoBispo on 18/03/2018.
//

#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"
#include "../ClavaDataDumper/ClavaDataDumper.h"

#include "llvm/ADT/STLForwardCompat.h"

#include <map>
#include "../Clava/HandlerCoverage.h"
#include <string>

// Data dumper selected directly by class name. Most entries dump with a
// method named after their own class; entries whose data section differs use
// DECL_DATA_ENTRY_AS(CLASS, SECTION).
#define DECL_DATA_ENTRY(CLASS)                                                 \
  {#CLASS, {#CLASS, [](clava::ClavaDataDumper &self, const Decl *D) {          \
    self.Dump##CLASS##Data(static_cast<const CLASS *>(D));                     \
  }}}

#define DECL_DATA_ENTRY_AS(CLASS, SECTION)                                     \
  {#CLASS, {#SECTION, [](clava::ClavaDataDumper &self, const Decl *D) {        \
    self.Dump##SECTION##Data(static_cast<const CLASS *>(D));                   \
  }}}

const std::map<std::string, clava::ClavaDataDumper::DeclDataEntry>
    clava::ClavaDataDumper::DECL_DATA_DUMPERS = {
        DECL_DATA_ENTRY(CXXConstructorDecl),
        DECL_DATA_ENTRY(CXXConversionDecl),
        DECL_DATA_ENTRY_AS(CXXDestructorDecl, CXXMethodDecl),
        DECL_DATA_ENTRY(CXXMethodDecl),
        DECL_DATA_ENTRY(FieldDecl),
        DECL_DATA_ENTRY(FunctionDecl),
        DECL_DATA_ENTRY_AS(ObjCImplementationDecl, NamedDecl),
        DECL_DATA_ENTRY(ParmVarDecl),
        DECL_DATA_ENTRY_AS(ClassTemplateDecl, TemplateDecl), // NAMED_DECL
        DECL_DATA_ENTRY_AS(FunctionTemplateDecl, TemplateDecl),
        DECL_DATA_ENTRY_AS(TypeAliasTemplateDecl, TemplateDecl),
        DECL_DATA_ENTRY_AS(VarTemplateDecl, TemplateDecl),
        DECL_DATA_ENTRY(TemplateTypeParmDecl),
        DECL_DATA_ENTRY(TypeDecl),
        DECL_DATA_ENTRY(UnresolvedUsingTypenameDecl),
        DECL_DATA_ENTRY(EnumDecl),
        DECL_DATA_ENTRY(RecordDecl),
        DECL_DATA_ENTRY(CXXRecordDecl),
        DECL_DATA_ENTRY(ClassTemplateSpecializationDecl),
        DECL_DATA_ENTRY(ClassTemplatePartialSpecializationDecl),
        DECL_DATA_ENTRY(VarDecl),
        DECL_DATA_ENTRY_AS(EnumConstantDecl, ValueDecl),
        DECL_DATA_ENTRY(NonTypeTemplateParmDecl),
        DECL_DATA_ENTRY_AS(UsingShadowDecl, NamedDecl),
        DECL_DATA_ENTRY_AS(TypeAliasDecl, TypedefNameDecl),
        DECL_DATA_ENTRY_AS(TypedefDecl, TypedefNameDecl),
        DECL_DATA_ENTRY(AccessSpecDecl),
        DECL_DATA_ENTRY(UsingDirectiveDecl),
        DECL_DATA_ENTRY(NamespaceDecl),
        DECL_DATA_ENTRY(NamespaceAliasDecl),
        DECL_DATA_ENTRY(LinkageSpecDecl),
        DECL_DATA_ENTRY_AS(LabelDecl, NamedDecl),
        DECL_DATA_ENTRY(StaticAssertDecl),
        DECL_DATA_ENTRY(TemplateTemplateParmDecl),
        DECL_DATA_ENTRY(MSPropertyDecl),
        DECL_DATA_ENTRY(UsingDecl),
        // TODO: Check if needs more data to dump
        DECL_DATA_ENTRY_AS(VarTemplateSpecializationDecl, VarDecl),
};

void clava::ClavaDataDumper::dump(const Decl *D) {
    const std::string classname = clava::getClassName(D);
    auto it = DECL_DATA_DUMPERS.find(classname);
    const char *dataName =
        it != DECL_DATA_DUMPERS.end() ? it->second.dataName : "Decl";

    // Dump header
    llvm::errs() << "<" << dataName << "Data>\n";
    llvm::errs() << clava::getId(D, id) << "\n";
    llvm::errs() << clava::getClassName(D) << "\n";

    if (it != DECL_DATA_DUMPERS.end()) {
        it->second.dump(*this, D);
    } else {
        clava::recordHandlerFallback("decl data", classname);
        // Default: plain Decl data
        DumpDeclData(D);
    }
}

void clava::ClavaDataDumper::DumpDeclData(const Decl *D) {
    clava::dumpSourceInfo(Context, D->getBeginLoc(), D->getEndLoc());

    // Print information about Decl
    clava::dump(D->isImplicit());
    clava::dump(D->isUsed());
    clava::dump(D->isReferenced());
    clava::dump(D->isInvalidDecl());
    clava::dump(D->isModulePrivate());

    // Attributes
    std::vector<std::string> attributesIds;
    for (Decl::attr_iterator I = D->attr_begin(), E = D->attr_end(); I != E;
         ++I) {
        attributesIds.push_back(clava::getId(*I, id));
    }
    clava::dump(attributesIds);
}

void clava::ClavaDataDumper::DumpNamedDeclData(const NamedDecl *D) {
    // Hierarchy
    DumpDeclData(D);

    // Print information about NamedDecl
    clava::dump(clava::getQualifiedPrefix(D));
    clava::dump(D->getDeclName().getAsString());
    clava::dump(D->getDeclName().getNameKind());

    clava::dump(D->isCXXClassMember());
    clava::dump(D->isCXXInstanceMember());
    clava::dump(
        clava::LINKAGE[llvm::to_underlying(D->getFormalLinkage())]);
    clava::dump(clava::VISIBILITY[D->getVisibility()]);
}

void clava::ClavaDataDumper::DumpTypeDeclData(const TypeDecl *D) {
    // Hierarchy
    DumpNamedDeclData(D);

    clava::dump(clava::getId(D->getTypeForDecl(), id));
}

void clava::ClavaDataDumper::DumpUnresolvedUsingTypenameDeclData(
    const UnresolvedUsingTypenameDecl *D) {
    // Hierarchy
    DumpTypeDeclData(D);

    clava::dump(D->getQualifier(), Context);
    clava::dump(D->isPackExpansion());
}

void clava::ClavaDataDumper::DumpTagDeclData(const TagDecl *D) {
    // Hierarchy
    DumpTypeDeclData(D);

    clava::dump(clava::TAG_KIND[llvm::to_underlying(D->getTagKind())]);
    clava::dump(D->isCompleteDefinition());
}

void clava::ClavaDataDumper::DumpEnumDeclData(const EnumDecl *D) {
    // Hierarchy
    DumpTagDeclData(D);

    // Dump EnumScopeType
    if (D->isScoped()) {
        if (D->isScopedUsingClassTag())
            clava::dump("CLASS");
        else
            clava::dump("STRUCT");
    } else {
        clava::dump("NO_SCOPE");
    }

    clava::dump(clava::getId(D->getIntegerType(), id));
}

void clava::ClavaDataDumper::DumpRecordDeclData(const RecordDecl *D) {
    // Hierarchy
    DumpTagDeclData(D);

    clava::dump(D->isAnonymousStructOrUnion());
}

void clava::ClavaDataDumper::DumpCXXRecordDeclData(const CXXRecordDecl *D) {
    // Hierarchy
    DumpRecordDeclData(D);

    if (D->hasDefinition()) {
        clava::dump(D->getNumBases());
        for (const auto &I : D->bases()) {
            clava::dump(I, id);
        }
    } else {
        clava::dump(0);
    }

    clava::dump(clava::getId(D->getDefinition(), id));
}

void clava::ClavaDataDumper::DumpClassTemplateSpecializationDeclData(
    const ClassTemplateSpecializationDecl *D) {
    // Hierarchy
    DumpCXXRecordDeclData(D);

    clava::dump(clava::getId(D->getSpecializedTemplate(), id));
    clava::dump(
        clava::TEMPLATE_SPECIALIZATION_KIND[D->getSpecializationKind()]);

    // Template specialization args
    auto &templateArgs = D->getTemplateArgs();
    clava::dump(templateArgs.size());
    for (auto &templateArg : templateArgs.asArray()) {
        clava::dump(templateArg, id, Context);
    }

    /** POSSIBLE ATTRIBUTES TO DUMP
    bool 	isExplicitSpecialization () const
    bool 	isClassScopeExplicitSpecialization () const
    bool 	isExplicitInstantiationOrSpecialization () const
    const TemplateArgumentList & 	getTemplateInstantiationArgs () const
     */
}

void clava::ClavaDataDumper::DumpClassTemplatePartialSpecializationDeclData(
    const ClassTemplatePartialSpecializationDecl *D) {
    // Hierarchy
    DumpClassTemplateSpecializationDeclData(D);

    /** POSSIBLE ATTRIBUTES TO DUMP
    TemplateParameterList * 	getTemplateParameters () const
    bool 	hasAssociatedConstraints () const
    const ASTTemplateArgumentListInfo * 	getTemplateArgsAsWritten ()
    const bool 	isMemberSpecialization () QualType
    getInjectedSpecializationType () const
    */
}

void clava::ClavaDataDumper::DumpValueDeclData(const ValueDecl *D) {
    // Hierarchy
    DumpNamedDeclData(D);

    clava::dump(D->getType(), id);
    clava::dump(D->isWeak());
}

void clava::ClavaDataDumper::DumpDeclaratorDeclData(const DeclaratorDecl *D) {
    // Hierarchy
    DumpValueDeclData(D);

    // Nothing for now
}

void clava::ClavaDataDumper::DumpFieldDeclData(const FieldDecl *D) {
    // Hierarchy
    DumpDeclaratorDeclData(D);

    clava::dump(D->isMutable());
}

void clava::ClavaDataDumper::DumpFunctionDeclData(const FunctionDecl *D) {
    // Hierarchy
    DumpDeclaratorDeclData(D);

    // Print information about FunctionDecl
    clava::dump(D->isConstexpr());
    clava::dump(D->getTemplatedKind());
    clava::dump(clava::STORAGE_CLASS[D->getStorageClass()]);
    clava::dump(D->isInlineSpecified());
    clava::dump(D->isVirtualAsWritten());
    clava::dump(D->isPureVirtual());
    clava::dump(D->isDeletedAsWritten());
    clava::dump(D->isExplicitlyDefaulted());

    clava::dump(clava::getId(D->getPreviousDecl(), id));
    clava::dump(clava::getId(D->getCanonicalDecl(), id));

    FunctionTemplateDecl *primaryTemplate = D->getPrimaryTemplate();
    if (primaryTemplate != nullptr) {
        clava::dump(clava::getId(primaryTemplate->getTemplatedDecl(), id));
    } else {
        clava::dump(clava::getId((const Decl *)nullptr, id));
    }

    // Template specialization args
    auto templateSpecializationArgs = D->getTemplateSpecializationArgs();
    if (templateSpecializationArgs != nullptr) {
        clava::dump(templateSpecializationArgs->size());
        for (auto templateArg : templateSpecializationArgs->asArray()) {
            clava::dump(templateArg, id, Context);
        }
    } else {
        clava::dump(0);
    }
}

void clava::ClavaDataDumper::DumpCXXMethodDeclData(const CXXMethodDecl *D) {
    // Hierarchy
    DumpFunctionDeclData(D);

    clava::dump(clava::getId(D->getParent(), id));

    clava::dump(D->size_overridden_methods());
    for (auto overriddenMethod : D->overridden_methods()) {
        clava::dump(clava::getId(overriddenMethod, id));
    }

    clava::dump(D->isStatic());
    clava::dump(D->isInstance());
    clava::dump(D->isConst());
    clava::dump(D->isVolatile());
    clava::dump(D->isVirtual());
    clava::dump(D->isCopyAssignmentOperator());
    clava::dump(D->isMoveAssignmentOperator());

    if (D->isInstance()) {
        clava::dump(clava::getId(D->getThisType(), id));
        clava::dump(clava::getId(D->getFunctionObjectParameterType(), id));
    } else {
        clava::dump(clava::getId((const Type *)nullptr, id));
        clava::dump(clava::getId((const Type *)nullptr, id));
    }

    // RefQualifier?

    clava::dump(D->hasInlineBody());
    clava::dump(D->isLambdaStaticInvoker());
}

void clava::ClavaDataDumper::DumpCXXConstructorDeclData(
    const CXXConstructorDecl *D) {
    // Hierarchy
    DumpCXXMethodDeclData(D);

    // Dump CXXCtorInitializers
    clava::dump(D->getNumCtorInitializers());
    for (auto init = D->init_begin(), init_last = D->init_end();
         init != init_last; ++init) {

        if ((*init)->isAnyMemberInitializer()) {
            clava::dump("ANY_MEMBER_INITIALIZER");
            clava::dump(clava::getId((*init)->getAnyMember(), id));
        } else if ((*init)->isBaseInitializer()) {
            clava::dump("BASE_INITIALIZER");
            clava::dump(clava::getId((*init)->getBaseClass(), id));
        } else if ((*init)->isDelegatingInitializer()) {
            clava::dump("DELEGATING_INITIALIZER");
            clava::dump(
                clava::getId((*init)->getTypeSourceInfo()->getType(), id));
        } else {
            throw std::invalid_argument(
                "ClangDataDumper::DumpCXXConstructorDeclData():: "
                "CXXCtorInitializer case not implemented");
        }

        // Init expr
        clava::dump(clava::getId((*init)->getInit(), id));

        clava::dump((*init)->isInClassMemberInitializer());
        clava::dump((*init)->isWritten());
    }

    clava::dump(D->isDefaultConstructor());
    clava::dump(D->isExplicit());
    clava::dump(D->getExplicitSpecifier(), id);
}

void clava::ClavaDataDumper::DumpCXXConversionDeclData(
    const CXXConversionDecl *D) {
    // Hierarchy
    DumpCXXMethodDeclData(D);

    clava::dump(D->isExplicit());
    clava::dump(D->isLambdaToBlockPointerConversion());

    clava::dump(D->getConversionType(), id);
}

void clava::ClavaDataDumper::DumpVarDeclData(const VarDecl *D) {
    // Hierarchy
    DumpValueDeclData(D);

    // Print information about VarDecl
    clava::dump(clava::STORAGE_CLASS[D->getStorageClass()]);
    clava::dump(clava::TLS_KIND[D->getTLSKind()]);
    clava::dump(D->isNRVOVariable());
    clava::dump(clava::INIT_STYLE[D->getInitStyle()]);

    clava::dump(D->isConstexpr());
    clava::dump(D->isStaticDataMember());
    clava::dump(D->isOutOfLine());
    clava::dump(D->hasGlobalStorage());
}

void clava::ClavaDataDumper::DumpParmVarDeclData(const ParmVarDecl *D) {

    // Hierarchy
    DumpVarDeclData(D);

    // Print information about ParmVarDecl
    clava::dump(D->hasInheritedDefaultArg());
}

void clava::ClavaDataDumper::DumpTemplateTypeParmDeclData(
    const TemplateTypeParmDecl *D) {

    // Hierarchy
    DumpTypeDeclData(D);

    // Kind
    if (D->wasDeclaredWithTypename()) {
        clava::dump("TYPENAME");
    } else {
        clava::dump("CLASS");
    }

    clava::dump(D->isParameterPack());

    if (D->hasDefaultArgument()) {
        clava::dump(clava::getId(D->getDefaultArgument(), id));
    } else {
        clava::dump(clava::getId((const Type *)nullptr, id));
    }
}

void clava::ClavaDataDumper::DumpTypedefNameDeclData(const TypedefNameDecl *D) {

    // Hierarchy
    DumpTypeDeclData(D);

    clava::dump(clava::getId(D->getUnderlyingType(), id));
}

void clava::ClavaDataDumper::DumpAccessSpecDeclData(const AccessSpecDecl *D) {

    // Hierarchy
    DumpDeclData(D);

    clava::dump(clava::ACCESS_SPECIFIER[D->getAccess()]);
}

void clava::ClavaDataDumper::DumpUsingDeclData(const UsingDecl *D) {

    // Hierarchy
    DumpNamedDeclData(D);

    clava::dump(D->getQualifier(), id);
}

void clava::ClavaDataDumper::DumpUsingDirectiveDeclData(
    const UsingDirectiveDecl *D) {

    // Hierarchy
    DumpNamedDeclData(D);

    clava::dump(clava::getSource(Context, D->getSourceRange()));
    clava::dump(clava::getId(D->getNominatedNamespace(), id));
    clava::dump(clava::getId(D->getNominatedNamespaceAsWritten(), id));
}

void clava::ClavaDataDumper::DumpNamespaceDeclData(const NamespaceDecl *D) {

    // Hierarchy
    DumpNamedDeclData(D);

    clava::dump(clava::getSource(Context, D->getSourceRange()));
}

void clava::ClavaDataDumper::DumpNamespaceAliasDeclData(
    const NamespaceAliasDecl *D) {

    // Hierarchy
    DumpNamedDeclData(D);

    clava::dump(
        clava::getSource(Context, D->getQualifierLoc().getSourceRange()));
    clava::dump(clava::getId(D->getAliasedNamespace(), id));
}

void clava::ClavaDataDumper::DumpLinkageSpecDeclData(const LinkageSpecDecl *D) {

    // Hierarchy
    DumpDeclData(D);

    switch (D->getLanguage()) {
    case LinkageSpecLanguageIDs::C:
        clava::dump(clava::LINKAGE_LANGUAGE[0]);
        break;
    case LinkageSpecLanguageIDs::CXX:
        clava::dump(clava::LINKAGE_LANGUAGE[1]);
        break;
    default:
        throw std::invalid_argument("ClangDataDumper::DumpLinkageSpecDeclData()"
                                    ":: Case not implemented, '" +
                                    std::to_string(llvm::to_underlying(
                                        D->getLanguage())) +
                                    "'");
    }
}

void clava::ClavaDataDumper::DumpStaticAssertDeclData(
    const StaticAssertDecl *D) {

    // Hierarchy
    DumpDeclData(D);

    clava::dump(D->isFailed());
}

void clava::ClavaDataDumper::DumpTemplateTemplateParmDeclData(
    const TemplateTemplateParmDecl *D) {

    // Hierarchy
    DumpTemplateDeclData(D);

    clava::dump(D->hasDefaultArgument());
    if (D->hasDefaultArgument()) {
        clava::dump(D->getDefaultArgument().getArgument(), id, Context);
    }

    clava::dump(D->isParameterPack());
    clava::dump(D->isPackExpansion());
    clava::dump(D->isExpandedParameterPack());
}

void clava::ClavaDataDumper::DumpNonTypeTemplateParmDeclData(
    const NonTypeTemplateParmDecl *D) {

    // Hierarchy
    DumpDeclaratorDeclData(D);

    if (D->hasDefaultArgument()) {
        clava::dump(clava::getId(D->getDefaultArgument(), id));
    } else {
        clava::dump(clava::getId((const Expr *)nullptr, id));
    }

    clava::dump(D->defaultArgumentWasInherited());
    clava::dump(D->isParameterPack());
    clava::dump(D->isPackExpansion());
    clava::dump(D->isExpandedParameterPack());

    if (D->isExpandedParameterPack()) {
        // Dump number of expansion types
        clava::dump(D->getNumExpansionTypes());
        for (unsigned i = 0; i < D->getNumExpansionTypes(); i++) {
            clava::dump(clava::getId(D->getExpansionType(i), id));
        }
    } else {
        // So that is always has a number
        clava::dump(0);
    }
}

void clava::ClavaDataDumper::DumpTemplateDeclData(const TemplateDecl *D) {

    // Hierarchy
    DumpNamedDeclData(D);

    auto templateParams = D->getTemplateParameters();
    if (templateParams) {
        clava::dump(templateParams->size());
        for (auto I = templateParams->begin(), E = templateParams->end();
             I != E; ++I) {
            clava::dump(clava::getId(*I, id));
        }
    } else {
        clava::dump(0);
    }

    clava::dump(clava::getId(D->getTemplatedDecl(), id));
}

void clava::ClavaDataDumper::DumpMSPropertyDeclData(const MSPropertyDecl *D) {

    // Hierarchy
    DumpDeclaratorDeclData(D);

    if (D->hasGetter()) {
        clava::dump(D->getGetterId()->getName());
    } else {
        clava::dump(NO_VALUE_STRING);
    }

    if (D->hasSetter()) {
        clava::dump(D->getSetterId()->getName());
    } else {
        clava::dump(NO_VALUE_STRING);
    }
}
