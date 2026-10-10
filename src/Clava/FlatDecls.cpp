#include "FlatSupport.h"

#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"

#include <llvm/ADT/STLForwardCompat.h>

namespace clava::flat {

namespace {

template <typename T>
std::unique_ptr<T> declBase(const clang::Decl *decl, Context &c) {
  (void)decl;
  (void)c;
  return std::make_unique<T>();
}

// Preserve the numeric NameKind values expected by existing Clava consumers.
// Their enum predates Clang's ordering changes; fixing that is a separate API change.
fb::NameKind nameKind(clang::DeclarationName::NameKind kind) {
  return static_cast<fb::NameKind>(kind);
}

fb::TemplateKind templateKind(clang::FunctionDecl::TemplatedKind kind) {
  switch (kind) {
  case clang::FunctionDecl::TK_NonTemplate:
    return enumValue<fb::TemplateKind>("NON_TEMPLATE");
  case clang::FunctionDecl::TK_FunctionTemplate:
    return enumValue<fb::TemplateKind>("FUNCTION_TEMPLATE");
  case clang::FunctionDecl::TK_MemberSpecialization:
    return enumValue<fb::TemplateKind>("MEMBER_SPECIALIZATION");
  case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
    return enumValue<fb::TemplateKind>("FUNCTION_TEMPLATE_SPECIALIZATION");
  case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization:
    return enumValue<fb::TemplateKind>(
        "DEPENDENT_FUNCTION_TEMPLATE_SPECIALIZATION");
  case clang::FunctionDecl::TK_DependentNonTemplate:
    throw std::invalid_argument("DependentNonTemplate is not represented by the current Clava TemplateKind");
  }
  return enumValue<fb::TemplateKind>("NON_TEMPLATE");
}

} // namespace

std::unique_ptr<fb::DeclDataT> makeDeclData(const clang::Decl *decl,
                                            Context &c) {
  auto out = declBase<fb::DeclDataT>(decl, c);
  out->base = makeNodeData(decl->getBeginLoc(), decl->getEndLoc(), c);
  out->is_implicit = decl->isImplicit();
  out->is_used = decl->isUsed();
  out->is_referenced = decl->isReferenced();
  out->is_invalid_decl = decl->isInvalidDecl();
  out->is_module_private = decl->isModulePrivate();
  for (auto attr = decl->attr_begin(); attr != decl->attr_end(); ++attr)
    out->attributes.push_back(clava::getId(*attr, c.id));
  return out;
}

std::unique_ptr<fb::NamedDeclDataT> makeNamedDeclData(
    const clang::NamedDecl *decl, Context &c) {
  auto out = declBase<fb::NamedDeclDataT>(decl, c);
  out->base = makeDeclData(decl, c);
  out->qualified_prefix = clava::getQualifiedPrefix(decl);
  out->decl_name = decl->getDeclName().getAsString();
  out->name_kind = nameKind(decl->getDeclName().getNameKind());
  out->is_cxx_class_member = decl->isCXXClassMember();
  out->is_cxx_instance_member = decl->isCXXInstanceMember();
  out->linkage = enumValue<fb::Linkage>(
      clava::LINKAGE[llvm::to_underlying(decl->getFormalLinkage())]);
  out->visibility = enumValue<fb::Visibility>(
      clava::VISIBILITY[llvm::to_underlying(decl->getVisibility())]);
  return out;
}

std::unique_ptr<fb::TypeDeclDataT> makeTypeDeclData(
    const clang::TypeDecl *decl, Context &c) {
  auto out = declBase<fb::TypeDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->type_for_decl = clava::getId(decl->getTypeForDecl(), c.id);
  return out;
}

std::unique_ptr<fb::TagDeclDataT> makeTagDeclData(const clang::TagDecl *decl,
                                                  Context &c) {
  auto out = declBase<fb::TagDeclDataT>(decl, c);
  out->base = makeTypeDeclData(decl, c);
  out->tag_kind = enumValue<fb::TagKind>(
      clava::TAG_KIND[llvm::to_underlying(decl->getTagKind())]);
  out->is_complete_definition = decl->isCompleteDefinition();
  return out;
}

std::unique_ptr<fb::RecordDeclDataT> makeRecordDeclData(
    const clang::RecordDecl *decl, Context &c) {
  auto out = declBase<fb::RecordDeclDataT>(decl, c);
  out->base = makeTagDeclData(decl, c);
  out->is_anonymous = decl->isAnonymousStructOrUnion();
  return out;
}

std::unique_ptr<fb::ValueDeclDataT> makeValueDeclData(
    const clang::ValueDecl *decl, Context &c) {
  auto out = declBase<fb::ValueDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->type = clava::getId(decl->getType(), c.id);
  out->is_weak = decl->isWeak();
  return out;
}

std::unique_ptr<fb::DeclaratorDeclDataT> makeDeclaratorDeclData(
    const clang::DeclaratorDecl *decl, Context &c) {
  auto out = declBase<fb::DeclaratorDeclDataT>(decl, c);
  out->base = makeValueDeclData(decl, c);
  return out;
}

std::unique_ptr<fb::TemplateDeclDataT> makeTemplateDeclData(
    const clang::TemplateDecl *decl, Context &c) {
  auto out = declBase<fb::TemplateDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  if (auto params = decl->getTemplateParameters()) {
    for (auto *param : *params)
      out->template_parameters.push_back(clava::getId(param, c.id));
  }
  out->template_decl = clava::getId(decl->getTemplatedDecl(), c.id);
  return out;
}

std::unique_ptr<fb::FunctionDeclDataT> makeFunctionDeclData(
    const clang::FunctionDecl *decl, Context &c) {
  auto out = declBase<fb::FunctionDeclDataT>(decl, c);
  out->base = makeDeclaratorDeclData(decl, c);
  out->is_constexpr = decl->isConstexpr();
  out->template_kind = templateKind(decl->getTemplatedKind());
  out->storage_class = enumValue<fb::StorageClass>(
      clava::STORAGE_CLASS[decl->getStorageClass()]);
  out->is_inline_specified = decl->isInlineSpecified();
  out->is_virtual_as_written = decl->isVirtualAsWritten();
  out->is_pure = decl->isPureVirtual();
  out->is_deleted = decl->isDeletedAsWritten();
  out->is_explicitly_defaulted = decl->isExplicitlyDefaulted();
  out->previous_decl = clava::getId(decl->getPreviousDecl(), c.id);
  out->canonical_decl = clava::getId(decl->getCanonicalDecl(), c.id);
  out->primary_template_decl = -2;
  if (auto *primary = decl->getPrimaryTemplate())
    out->primary_template_decl =
        clava::getId(primary->getTemplatedDecl(), c.id);
  if (auto *args = decl->getTemplateSpecializationArgs()) {
    for (const auto &arg : args->asArray())
      out->template_arguments.push_back(makeTemplateArgument(arg, c));
  }
  for (unsigned listIndex = 0; listIndex < decl->getNumTemplateParameterLists();
       ++listIndex) {
    const auto *parameters = decl->getTemplateParameterList(listIndex);
    out->template_parameter_list_sizes.push_back(parameters->size());
    for (const auto *parameter : *parameters)
      out->template_parameters.push_back(
          clava::getId(parameter, c.id));
  }
  return out;
}

std::unique_ptr<fb::CXXMethodDeclDataT> makeCXXMethodDeclData(
    const clang::CXXMethodDecl *decl, Context &c) {
  auto out = declBase<fb::CXXMethodDeclDataT>(decl, c);
  out->base = makeFunctionDeclData(decl, c);
  out->record = clava::getId(decl->getParent(), c.id);
  for (auto *method : decl->overridden_methods())
    out->overridden_methods.push_back(clava::getId(method, c.id));
  out->is_static = decl->isStatic();
  out->is_instance = decl->isInstance();
  out->is_const = decl->isConst();
  out->is_volatile = decl->isVolatile();
  out->is_virtual = decl->isVirtual();
  out->is_copy_assignment_operator = decl->isCopyAssignmentOperator();
  out->is_move_assignment_operator = decl->isMoveAssignmentOperator();
  out->this_type = decl->isInstance()?clava::getId(decl->getThisType(), c.id):-1;
  out->this_object_type =
      decl->isInstance()?clava::getId(decl->getFunctionObjectParameterType(), c.id):-1;
  out->has_inline_body = decl->hasInlineBody();
  out->is_lambda_static_invoker = decl->isLambdaStaticInvoker();
  return out;
}

std::unique_ptr<fb::CXXConstructorDeclDataT> makeCXXConstructorDeclData(
    const clang::CXXConstructorDecl *decl, Context &c) {
  auto out = declBase<fb::CXXConstructorDeclDataT>(decl, c);
  out->base = makeCXXMethodDeclData(decl, c);
  for (auto init = decl->init_begin(); init != decl->init_end(); ++init)
    out->constructor_inits.push_back(makeCXXCtorInitializer(*init, c));
  out->is_default_constructor = decl->isDefaultConstructor();
  out->is_explicit = decl->isExplicit();
  out->explicit_specifier = makeExplicitSpecifier(decl->getExplicitSpecifier(), c);
  return out;
}

std::unique_ptr<fb::CXXConversionDeclDataT> makeCXXConversionDeclData(
    const clang::CXXConversionDecl *decl, Context &c) {
  auto out = declBase<fb::CXXConversionDeclDataT>(decl, c);
  out->base = makeCXXMethodDeclData(decl, c);
  out->is_explicit = decl->isExplicit();
  out->is_lambda_to_block_pointer_conversion =
      decl->isLambdaToBlockPointerConversion();
  out->conversion_type = clava::getId(decl->getConversionType(), c.id);
  return out;
}

std::unique_ptr<fb::FieldDeclDataT> makeFieldDeclData(
    const clang::FieldDecl *decl, Context &c) {
  auto out = declBase<fb::FieldDeclDataT>(decl, c);
  out->base = makeDeclaratorDeclData(decl, c);
  out->is_mutable = decl->isMutable();
  return out;
}

std::unique_ptr<fb::VarDeclDataT> makeVarDeclData(const clang::VarDecl *decl,
                                                  Context &c) {
  auto out = declBase<fb::VarDeclDataT>(decl, c);
  out->base = makeValueDeclData(decl, c);
  out->storage_class =
      enumValue<fb::StorageClass>(clava::STORAGE_CLASS[decl->getStorageClass()]);
  out->tls_kind = enumValue<fb::TLSKind>(clava::TLS_KIND[decl->getTLSKind()]);
  out->is_nrvo_variable = decl->isNRVOVariable();
  out->init_style =
      enumValue<fb::InitializationStyle>(clava::INIT_STYLE[decl->getInitStyle()]);
  out->is_constexpr = decl->isConstexpr();
  out->is_static_data_member = decl->isStaticDataMember();
  out->is_out_of_line = decl->isOutOfLine();
  out->has_global_storage = decl->hasGlobalStorage();
  return out;
}

std::unique_ptr<fb::ParmVarDeclDataT> makeParmVarDeclData(
    const clang::ParmVarDecl *decl, Context &c) {
  auto out = declBase<fb::ParmVarDeclDataT>(decl, c);
  out->base = makeVarDeclData(decl, c);
  out->has_inherited_default_arg = decl->hasInheritedDefaultArg();
  return out;
}

std::unique_ptr<fb::TemplateTypeParmDeclDataT> makeTemplateTypeParmDeclData(
    const clang::TemplateTypeParmDecl *decl, Context &c) {
  auto out = declBase<fb::TemplateTypeParmDeclDataT>(decl, c);
  out->base = makeTypeDeclData(decl, c);
  out->kind = enumValue<fb::TemplateTypeParmKind>(
      decl->wasDeclaredWithTypename() ? "TYPENAME" : "CLASS");
  out->is_parameter_pack = decl->isParameterPack();
  out->default_argument = decl->hasDefaultArgument() ? clava::getId(decl->getDefaultArgument(), c.id) : -1;
  return out;
}

std::unique_ptr<fb::UnresolvedUsingTypenameDeclDataT>
makeUnresolvedUsingTypenameDeclData(
    const clang::UnresolvedUsingTypenameDecl *decl, Context &c) {
  auto out = declBase<fb::UnresolvedUsingTypenameDeclDataT>(decl, c);
  out->base = makeTypeDeclData(decl, c);
  out->qualifier = qualifierString(decl->getQualifier(), c);
  out->is_pack_expansion = decl->isPackExpansion();
  return out;
}

std::unique_ptr<fb::EnumDeclDataT> makeEnumDeclData(const clang::EnumDecl *decl,
                                                    Context &c) {
  auto out = declBase<fb::EnumDeclDataT>(decl, c);
  out->base = makeTagDeclData(decl, c);
  if (decl->isScoped())
    out->enum_scope_kind = enumValue<fb::EnumScopeType>(
        decl->isScopedUsingClassTag() ? "CLASS" : "STRUCT");
  else
    out->enum_scope_kind = enumValue<fb::EnumScopeType>("NO_SCOPE");
  out->integer_type = clava::getId(decl->getIntegerType(), c.id);
  return out;
}

std::unique_ptr<fb::CXXRecordDeclDataT> makeCXXRecordDeclData(
    const clang::CXXRecordDecl *decl, Context &c) {
  auto out = declBase<fb::CXXRecordDeclDataT>(decl, c);
  out->base = makeRecordDeclData(decl, c);
  if (decl->hasDefinition())
    for (const auto &base : decl->bases())
      out->record_bases.push_back(makeCXXBaseSpecifier(base, c));
  out->record_definition = clava::getId(decl->getDefinition(), c.id);
  return out;
}

std::unique_ptr<fb::ClassTemplateSpecializationDeclDataT>
makeClassTemplateSpecializationDeclData(
    const clang::ClassTemplateSpecializationDecl *decl, Context &c) {
  auto out = declBase<fb::ClassTemplateSpecializationDeclDataT>(decl, c);
  out->base = makeCXXRecordDeclData(decl, c);
  out->specialized_template =
      clava::getId(decl->getSpecializedTemplate(), c.id);
  out->specialization_kind = enumValue<fb::TemplateSpecializationKind>(
      clava::TEMPLATE_SPECIALIZATION_KIND[decl->getSpecializationKind()]);
  for (const auto &arg : decl->getTemplateArgs().asArray())
    out->template_arguments.push_back(makeTemplateArgument(arg, c));
  return out;
}

std::unique_ptr<fb::NonTypeTemplateParmDeclDataT>
makeNonTypeTemplateParmDeclData(
    const clang::NonTypeTemplateParmDecl *decl, Context &c) {
  auto out = declBase<fb::NonTypeTemplateParmDeclDataT>(decl, c);
  out->base = makeDeclaratorDeclData(decl, c);
  out->default_argument = decl->hasDefaultArgument() ? clava::getId(decl->getDefaultArgument(), c.id) : -3;
  out->default_argument_was_inherited = decl->defaultArgumentWasInherited();
  out->is_parameter_pack = decl->isParameterPack();
  out->is_pack_expansion = decl->isPackExpansion();
  out->is_expanded_parameter_pack = decl->isExpandedParameterPack();
  if (decl->isExpandedParameterPack())
    for (unsigned i = 0; i < decl->getNumExpansionTypes(); ++i)
      out->expansion_types.push_back(clava::getId(
          decl->getExpansionType(i), c.id));
  return out;
}

std::unique_ptr<fb::TypedefNameDeclDataT> makeTypedefNameDeclData(
    const clang::TypedefNameDecl *decl, Context &c) {
  auto out = declBase<fb::TypedefNameDeclDataT>(decl, c);
  out->base = makeTypeDeclData(decl, c);
  out->underlying_type = clava::getId(decl->getUnderlyingType(), c.id);
  return out;
}

std::unique_ptr<fb::AccessSpecDeclDataT> makeAccessSpecDeclData(
    const clang::AccessSpecDecl *decl, Context &c) {
  auto out = declBase<fb::AccessSpecDeclDataT>(decl, c);
  out->base = makeDeclData(decl, c);
  out->access_specifier =
      enumValue<fb::AccessSpecifier>(clava::ACCESS_SPECIFIER[decl->getAccess()]);
  return out;
}

std::unique_ptr<fb::UsingDeclDataT> makeUsingDeclData(
    const clang::UsingDecl *decl, Context &c) {
  auto out = declBase<fb::UsingDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->nested_name_specifier = makeNestedNameSpecifier(decl->getQualifier(), c);
  return out;
}

std::unique_ptr<fb::UsingDirectiveDeclDataT> makeUsingDirectiveDeclData(
    const clang::UsingDirectiveDecl *decl, Context &c) {
  auto out = declBase<fb::UsingDirectiveDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->qualifier = sourceText(decl->getSourceRange(), c);
  out->namespace_ = clava::getId(decl->getNominatedNamespace(), c.id);
  out->namespace_as_written =
      clava::getId(decl->getNominatedNamespaceAsWritten(), c.id);
  return out;
}

std::unique_ptr<fb::NamespaceDeclDataT> makeNamespaceDeclData(
    const clang::NamespaceDecl *decl, Context &c) {
  auto out = declBase<fb::NamespaceDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->source_literal = sourceText(decl->getSourceRange(), c);
  return out;
}

std::unique_ptr<fb::NamespaceAliasDeclDataT> makeNamespaceAliasDeclData(
    const clang::NamespaceAliasDecl *decl, Context &c) {
  auto out = declBase<fb::NamespaceAliasDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->nested_prefix = sourceText(decl->getQualifierLoc().getSourceRange(), c);
  out->aliased_namespace =
      clava::getId(decl->getAliasedNamespace(), c.id);
  return out;
}

std::unique_ptr<fb::LinkageSpecDeclDataT> makeLinkageSpecDeclData(
    const clang::LinkageSpecDecl *decl, Context &c) {
  auto out = declBase<fb::LinkageSpecDeclDataT>(decl, c);
  out->base = makeDeclData(decl, c);
  switch (decl->getLanguage()) {
  case clang::LinkageSpecLanguageIDs::C:
    out->linkage_type = enumValue<fb::LanguageId>("C");
    break;
  case clang::LinkageSpecLanguageIDs::CXX:
    out->linkage_type = enumValue<fb::LanguageId>("CXX");
    break;
  }
  return out;
}

std::unique_ptr<fb::StaticAssertDeclDataT> makeStaticAssertDeclData(
    const clang::StaticAssertDecl *decl, Context &c) {
  auto out = declBase<fb::StaticAssertDeclDataT>(decl, c);
  out->base = makeDeclData(decl, c);
  out->is_failed = decl->isFailed();
  return out;
}

std::unique_ptr<fb::TemplateTemplateParmDeclDataT>
makeTemplateTemplateParmDeclData(
    const clang::TemplateTemplateParmDecl *decl, Context &c) {
  auto out = declBase<fb::TemplateTemplateParmDeclDataT>(decl, c);
  out->base = makeTemplateDeclData(decl, c);
  if (decl->hasDefaultArgument())
    out->default_argument =
        makeTemplateArgument(decl->getDefaultArgument().getArgument(), c);
  out->is_parameter_pack = decl->isParameterPack();
  out->is_pack_expansion = decl->isPackExpansion();
  out->is_expanded_parameter_pack = decl->isExpandedParameterPack();
  return out;
}

std::unique_ptr<fb::MSPropertyDeclDataT> makeMSPropertyDeclData(
    const clang::MSPropertyDecl *decl, Context &c) {
  auto out = declBase<fb::MSPropertyDeclDataT>(decl, c);
  out->base = makeDeclaratorDeclData(decl, c);
  out->getter_name = decl->hasGetter() ? decl->getGetterId()->getName().str()
                                       : std::string();
  out->setter_name = decl->hasSetter() ? decl->getSetterId()->getName().str()
                                       : std::string();
  return out;
}

// Native dispatch aliases.  These entries intentionally return the data table
// of the existing handler section, while the outer Node retains the concrete
// Clang class name.
std::unique_ptr<fb::CXXMethodDeclDataT> makeCXXDestructorDeclData(
    const clang::CXXDestructorDecl *decl, Context &c) {
  return makeCXXMethodDeclData(decl, c);
}

std::unique_ptr<fb::NamedDeclDataT> makeObjCImplementationDeclData(
    const clang::ObjCImplementationDecl *decl, Context &c) {
  return makeNamedDeclData(decl, c);
}

std::unique_ptr<fb::TemplateDeclDataT> makeClassTemplateDeclData(
    const clang::ClassTemplateDecl *decl, Context &c) {
  return makeTemplateDeclData(decl, c);
}
std::unique_ptr<fb::TemplateDeclDataT> makeFunctionTemplateDeclData(
    const clang::FunctionTemplateDecl *decl, Context &c) {
  return makeTemplateDeclData(decl, c);
}
std::unique_ptr<fb::TemplateDeclDataT> makeTypeAliasTemplateDeclData(
    const clang::TypeAliasTemplateDecl *decl, Context &c) {
  return makeTemplateDeclData(decl, c);
}
std::unique_ptr<fb::TemplateDeclDataT> makeVarTemplateDeclData(
    const clang::VarTemplateDecl *decl, Context &c) {
  return makeTemplateDeclData(decl, c);
}
std::unique_ptr<fb::ValueDeclDataT> makeEnumConstantDeclData(
    const clang::EnumConstantDecl *decl, Context &c) {
  return makeValueDeclData(decl, c);
}
std::unique_ptr<fb::NamedDeclDataT> makeUsingShadowDeclData(
    const clang::UsingShadowDecl *decl, Context &c) {
  return makeNamedDeclData(decl, c);
}
std::unique_ptr<fb::TypedefNameDeclDataT> makeTypeAliasDeclData(
    const clang::TypeAliasDecl *decl, Context &c) {
  return makeTypedefNameDeclData(decl, c);
}
std::unique_ptr<fb::TypedefNameDeclDataT> makeTypedefDeclData(
    const clang::TypedefDecl *decl, Context &c) {
  return makeTypedefNameDeclData(decl, c);
}
std::unique_ptr<fb::NamedDeclDataT> makeLabelDeclData(
    const clang::LabelDecl *decl, Context &c) {
  return makeNamedDeclData(decl, c);
}
std::unique_ptr<fb::VarDeclDataT> makeVarTemplateSpecializationDeclData(
    const clang::VarTemplateSpecializationDecl *decl, Context &c) {
  return makeVarDeclData(decl, c);
}

std::unique_ptr<fb::ClassTemplatePartialSpecializationDeclDataT>
makeClassTemplatePartialSpecializationDeclData(
    const clang::ClassTemplatePartialSpecializationDecl *decl, Context &c) {
  auto out = std::make_unique<fb::ClassTemplatePartialSpecializationDeclDataT>();
  out->base = makeClassTemplateSpecializationDeclData(decl, c);
  if (auto *params = decl->getTemplateParameters())
    for (auto *param : *params)
      out->template_parameters.push_back(clava::getId(param, c.id));
  return out;
}

/** Payload builder per emitted Clang class; other classes use the base Decl payload. */
fb::NodeT makeNode(const clang::Decl *node, Context &c) {
  auto out = nodeHeader(node, c);
  FLAT_PAYLOAD(CXXConstructorDecl, makeCXXConstructorDeclData)
  FLAT_PAYLOAD(CXXConversionDecl, makeCXXConversionDeclData)
  FLAT_PAYLOAD(CXXDestructorDecl, makeCXXMethodDeclData)
  FLAT_PAYLOAD(CXXMethodDecl, makeCXXMethodDeclData)
  FLAT_PAYLOAD(FieldDecl, makeFieldDeclData)
  FLAT_PAYLOAD(FunctionDecl, makeFunctionDeclData)
  FLAT_PAYLOAD(ObjCImplementationDecl, makeNamedDeclData)
  FLAT_PAYLOAD(ParmVarDecl, makeParmVarDeclData)
  FLAT_PAYLOAD(ClassTemplateDecl, makeTemplateDeclData)
  FLAT_PAYLOAD(FunctionTemplateDecl, makeTemplateDeclData)
  FLAT_PAYLOAD(TypeAliasTemplateDecl, makeTemplateDeclData)
  FLAT_PAYLOAD(VarTemplateDecl, makeTemplateDeclData)
  FLAT_PAYLOAD(TemplateTypeParmDecl, makeTemplateTypeParmDeclData)
  FLAT_PAYLOAD(TypeDecl, makeTypeDeclData)
  FLAT_PAYLOAD(UnresolvedUsingTypenameDecl, makeUnresolvedUsingTypenameDeclData)
  FLAT_PAYLOAD(EnumDecl, makeEnumDeclData)
  FLAT_PAYLOAD(RecordDecl, makeRecordDeclData)
  FLAT_PAYLOAD(CXXRecordDecl, makeCXXRecordDeclData)
  FLAT_PAYLOAD(ClassTemplateSpecializationDecl, makeClassTemplateSpecializationDeclData)
  FLAT_PAYLOAD(ClassTemplatePartialSpecializationDecl, makeClassTemplatePartialSpecializationDeclData)
  FLAT_PAYLOAD(VarDecl, makeVarDeclData)
  FLAT_PAYLOAD(EnumConstantDecl, makeValueDeclData)
  FLAT_PAYLOAD(NonTypeTemplateParmDecl, makeNonTypeTemplateParmDeclData)
  FLAT_PAYLOAD(UsingShadowDecl, makeNamedDeclData)
  FLAT_PAYLOAD(TypeAliasDecl, makeTypedefNameDeclData)
  FLAT_PAYLOAD(TypedefDecl, makeTypedefNameDeclData)
  FLAT_PAYLOAD(AccessSpecDecl, makeAccessSpecDeclData)
  FLAT_PAYLOAD(UsingDirectiveDecl, makeUsingDirectiveDeclData)
  FLAT_PAYLOAD(NamespaceDecl, makeNamespaceDeclData)
  FLAT_PAYLOAD(NamespaceAliasDecl, makeNamespaceAliasDeclData)
  FLAT_PAYLOAD(LinkageSpecDecl, makeLinkageSpecDeclData)
  FLAT_PAYLOAD(LabelDecl, makeNamedDeclData)
  FLAT_PAYLOAD(StaticAssertDecl, makeStaticAssertDeclData)
  FLAT_PAYLOAD(TemplateTemplateParmDecl, makeTemplateTemplateParmDeclData)
  FLAT_PAYLOAD(MSPropertyDecl, makeMSPropertyDeclData)
  FLAT_PAYLOAD(UsingDecl, makeUsingDeclData)
  FLAT_PAYLOAD(VarTemplateSpecializationDecl, makeVarDeclData)
  out.payload.Set(std::move(*makeDeclData(node, c)));
  return out;
}

} // namespace clava::flat
