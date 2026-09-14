#include "ProtoSupport.h"

#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"

#include <llvm/ADT/STLForwardCompat.h>

namespace clava::proto {

namespace {

template <typename T>
std::unique_ptr<T> declBase(const clang::Decl *decl, Context &c) {
  (void)decl;
  (void)c;
  return std::make_unique<T>();
}

// Preserve the numeric NameKind values exported by the existing text protocol.
// Its Java enum predates Clang's ordering changes; fixing that is a separate API change.
obj::NameKind nameKind(clang::DeclarationName::NameKind kind) {
  return static_cast<obj::NameKind>(kind);
}

obj::TemplateKind templateKind(clang::FunctionDecl::TemplatedKind kind) {
  switch (kind) {
  case clang::FunctionDecl::TK_NonTemplate:
    return enumValue<obj::TemplateKind>("NON_TEMPLATE");
  case clang::FunctionDecl::TK_FunctionTemplate:
    return enumValue<obj::TemplateKind>("FUNCTION_TEMPLATE");
  case clang::FunctionDecl::TK_MemberSpecialization:
    return enumValue<obj::TemplateKind>("MEMBER_SPECIALIZATION");
  case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
    return enumValue<obj::TemplateKind>("FUNCTION_TEMPLATE_SPECIALIZATION");
  case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization:
    return enumValue<obj::TemplateKind>(
        "DEPENDENT_FUNCTION_TEMPLATE_SPECIALIZATION");
  case clang::FunctionDecl::TK_DependentNonTemplate:
    throw std::invalid_argument("DependentNonTemplate is not represented by the current Clava TemplateKind");
  }
  return enumValue<obj::TemplateKind>("NON_TEMPLATE");
}

} // namespace

std::unique_ptr<obj::DeclDataT> makeDeclData(const clang::Decl *decl,
                                            Context &c) {
  auto out = declBase<obj::DeclDataT>(decl, c);
  out->base = makeNodeData(decl->getBeginLoc(), decl->getEndLoc(), c);
  out->is_implicit = decl->isImplicit();
  out->is_used = decl->isUsed();
  out->is_referenced = decl->isReferenced();
  out->is_invalid_decl = decl->isInvalidDecl();
  out->is_module_private = decl->isModulePrivate();
  for (auto attr = decl->attr_begin(); attr != decl->attr_end(); ++attr)
    out->attributes.push_back(wireId(clava::getId(*attr, c.id)));
  return out;
}

std::unique_ptr<obj::NamedDeclDataT> makeNamedDeclData(
    const clang::NamedDecl *decl, Context &c) {
  auto out = declBase<obj::NamedDeclDataT>(decl, c);
  out->base = makeDeclData(decl, c);
  out->qualified_prefix = clava::getQualifiedPrefix(decl);
  out->decl_name = decl->getDeclName().getAsString();
  out->name_kind = nameKind(decl->getDeclName().getNameKind());
  out->is_cxx_class_member = decl->isCXXClassMember();
  out->is_cxx_instance_member = decl->isCXXInstanceMember();
  out->linkage = enumValue<obj::Linkage>(
      clava::LINKAGE[llvm::to_underlying(decl->getFormalLinkage())]);
  out->visibility = enumValue<obj::Visibility>(
      clava::VISIBILITY[llvm::to_underlying(decl->getVisibility())]);
  return out;
}

std::unique_ptr<obj::TypeDeclDataT> makeTypeDeclData(
    const clang::TypeDecl *decl, Context &c) {
  auto out = declBase<obj::TypeDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->type_for_decl = wireId(clava::getId(decl->getTypeForDecl(), c.id));
  return out;
}

std::unique_ptr<obj::TagDeclDataT> makeTagDeclData(const clang::TagDecl *decl,
                                                  Context &c) {
  auto out = declBase<obj::TagDeclDataT>(decl, c);
  out->base = makeTypeDeclData(decl, c);
  out->tag_kind = enumValue<obj::TagKind>(
      clava::TAG_KIND[llvm::to_underlying(decl->getTagKind())]);
  out->is_complete_definition = decl->isCompleteDefinition();
  return out;
}

std::unique_ptr<obj::RecordDeclDataT> makeRecordDeclData(
    const clang::RecordDecl *decl, Context &c) {
  auto out = declBase<obj::RecordDeclDataT>(decl, c);
  out->base = makeTagDeclData(decl, c);
  out->is_anonymous = decl->isAnonymousStructOrUnion();
  return out;
}

std::unique_ptr<obj::ValueDeclDataT> makeValueDeclData(
    const clang::ValueDecl *decl, Context &c) {
  auto out = declBase<obj::ValueDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->type = wireId(clava::getId(decl->getType(), c.id));
  out->is_weak = decl->isWeak();
  return out;
}

std::unique_ptr<obj::DeclaratorDeclDataT> makeDeclaratorDeclData(
    const clang::DeclaratorDecl *decl, Context &c) {
  auto out = declBase<obj::DeclaratorDeclDataT>(decl, c);
  out->base = makeValueDeclData(decl, c);
  return out;
}

std::unique_ptr<obj::TemplateDeclDataT> makeTemplateDeclData(
    const clang::TemplateDecl *decl, Context &c) {
  auto out = declBase<obj::TemplateDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  if (auto params = decl->getTemplateParameters()) {
    for (auto *param : *params)
      out->template_parameters.push_back(wireId(clava::getId(param, c.id)));
  }
  out->templated_decl = wireId(clava::getId(decl->getTemplatedDecl(), c.id));
  return out;
}

std::unique_ptr<obj::FunctionDeclDataT> makeFunctionDeclData(
    const clang::FunctionDecl *decl, Context &c) {
  auto out = declBase<obj::FunctionDeclDataT>(decl, c);
  out->base = makeDeclaratorDeclData(decl, c);
  out->is_constexpr = decl->isConstexpr();
  out->template_kind = templateKind(decl->getTemplatedKind());
  out->storage_class = enumValue<obj::StorageClass>(
      clava::STORAGE_CLASS[decl->getStorageClass()]);
  out->is_inline_specified = decl->isInlineSpecified();
  out->is_virtual_as_written = decl->isVirtualAsWritten();
  out->is_pure = decl->isPureVirtual();
  out->is_deleted = decl->isDeletedAsWritten();
  out->is_explicitly_defaulted = decl->isExplicitlyDefaulted();
  out->previous_decl = wireId(clava::getId(decl->getPreviousDecl(), c.id));
  out->canonical_decl = wireId(clava::getId(decl->getCanonicalDecl(), c.id));
  out->primary_template_decl = -2;
  if (auto *primary = decl->getPrimaryTemplate())
    out->primary_template_decl =
        wireId(clava::getId(primary->getTemplatedDecl(), c.id));
  if (auto *args = decl->getTemplateSpecializationArgs()) {
    for (const auto &arg : args->asArray())
      out->template_arguments.push_back(makeTemplateArgument(arg, c));
  }
  return out;
}

std::unique_ptr<obj::CXXMethodDeclDataT> makeCXXMethodDeclData(
    const clang::CXXMethodDecl *decl, Context &c) {
  auto out = declBase<obj::CXXMethodDeclDataT>(decl, c);
  out->base = makeFunctionDeclData(decl, c);
  out->record = wireId(clava::getId(decl->getParent(), c.id));
  for (auto *method : decl->overridden_methods())
    out->overridden_methods.push_back(wireId(clava::getId(method, c.id)));
  out->is_static = decl->isStatic();
  out->is_instance = decl->isInstance();
  out->is_const = decl->isConst();
  out->is_volatile = decl->isVolatile();
  out->is_virtual = decl->isVirtual();
  out->is_copy_assignment_operator = decl->isCopyAssignmentOperator();
  out->is_move_assignment_operator = decl->isMoveAssignmentOperator();
  out->this_type = decl->isInstance()?wireId(clava::getId(decl->getThisType(), c.id)):-1;
  out->this_object_type =
      decl->isInstance()?wireId(clava::getId(decl->getFunctionObjectParameterType(), c.id)):-1;
  out->has_inline_body = decl->hasInlineBody();
  out->is_lambda_static_invoker = decl->isLambdaStaticInvoker();
  return out;
}

std::unique_ptr<obj::CXXConstructorDeclDataT> makeCXXConstructorDeclData(
    const clang::CXXConstructorDecl *decl, Context &c) {
  auto out = declBase<obj::CXXConstructorDeclDataT>(decl, c);
  out->base = makeCXXMethodDeclData(decl, c);
  for (auto init = decl->init_begin(); init != decl->init_end(); ++init)
    out->constructor_inits.push_back(makeCXXCtorInitializer(*init, c));
  out->is_default_constructor = decl->isDefaultConstructor();
  out->is_explicit = decl->isExplicit();
  out->explicit_specifier = makeExplicitSpecifier(decl->getExplicitSpecifier(), c);
  return out;
}

std::unique_ptr<obj::CXXConversionDeclDataT> makeCXXConversionDeclData(
    const clang::CXXConversionDecl *decl, Context &c) {
  auto out = declBase<obj::CXXConversionDeclDataT>(decl, c);
  out->base = makeCXXMethodDeclData(decl, c);
  out->is_explicit = decl->isExplicit();
  out->is_lambda_to_block_pointer_conversion =
      decl->isLambdaToBlockPointerConversion();
  out->conversion_type = wireId(clava::getId(decl->getConversionType(), c.id));
  return out;
}

std::unique_ptr<obj::FieldDeclDataT> makeFieldDeclData(
    const clang::FieldDecl *decl, Context &c) {
  auto out = declBase<obj::FieldDeclDataT>(decl, c);
  out->base = makeDeclaratorDeclData(decl, c);
  out->is_mutable = decl->isMutable();
  return out;
}

std::unique_ptr<obj::VarDeclDataT> makeVarDeclData(const clang::VarDecl *decl,
                                                  Context &c) {
  auto out = declBase<obj::VarDeclDataT>(decl, c);
  out->base = makeValueDeclData(decl, c);
  out->storage_class =
      enumValue<obj::StorageClass>(clava::STORAGE_CLASS[decl->getStorageClass()]);
  out->tls_kind = enumValue<obj::TLSKind>(clava::TLS_KIND[decl->getTLSKind()]);
  out->is_nrvo_variable = decl->isNRVOVariable();
  out->init_style =
      enumValue<obj::InitializationStyle>(clava::INIT_STYLE[decl->getInitStyle()]);
  out->is_constexpr = decl->isConstexpr();
  out->is_static_data_member = decl->isStaticDataMember();
  out->is_out_of_line = decl->isOutOfLine();
  out->has_global_storage = decl->hasGlobalStorage();
  return out;
}

std::unique_ptr<obj::ParmVarDeclDataT> makeParmVarDeclData(
    const clang::ParmVarDecl *decl, Context &c) {
  auto out = declBase<obj::ParmVarDeclDataT>(decl, c);
  out->base = makeVarDeclData(decl, c);
  out->has_inherited_default_arg = decl->hasInheritedDefaultArg();
  return out;
}

std::unique_ptr<obj::TemplateTypeParmDeclDataT> makeTemplateTypeParmDeclData(
    const clang::TemplateTypeParmDecl *decl, Context &c) {
  auto out = declBase<obj::TemplateTypeParmDeclDataT>(decl, c);
  out->base = makeTypeDeclData(decl, c);
  out->kind = enumValue<obj::TemplateTypeParmKind>(
      decl->wasDeclaredWithTypename() ? "TYPENAME" : "CLASS");
  out->is_parameter_pack = decl->isParameterPack();
  out->default_argument = decl->hasDefaultArgument() ? wireId(clava::getId(decl->getDefaultArgument(), c.id)) : -1;
  return out;
}

std::unique_ptr<obj::UnresolvedUsingTypenameDeclDataT>
makeUnresolvedUsingTypenameDeclData(
    const clang::UnresolvedUsingTypenameDecl *decl, Context &c) {
  auto out = declBase<obj::UnresolvedUsingTypenameDeclDataT>(decl, c);
  out->base = makeTypeDeclData(decl, c);
  out->qualifier = qualifierString(decl->getQualifier(), c);
  out->is_pack_expansion = decl->isPackExpansion();
  return out;
}

std::unique_ptr<obj::EnumDeclDataT> makeEnumDeclData(const clang::EnumDecl *decl,
                                                    Context &c) {
  auto out = declBase<obj::EnumDeclDataT>(decl, c);
  out->base = makeTagDeclData(decl, c);
  if (decl->isScoped())
    out->enum_scope_kind = enumValue<obj::EnumScopeType>(
        decl->isScopedUsingClassTag() ? "CLASS" : "STRUCT");
  else
    out->enum_scope_kind = enumValue<obj::EnumScopeType>("NO_SCOPE");
  out->integer_type = wireId(clava::getId(decl->getIntegerType(), c.id));
  return out;
}

std::unique_ptr<obj::CXXRecordDeclDataT> makeCXXRecordDeclData(
    const clang::CXXRecordDecl *decl, Context &c) {
  auto out = declBase<obj::CXXRecordDeclDataT>(decl, c);
  out->base = makeRecordDeclData(decl, c);
  if (decl->hasDefinition())
    for (const auto &base : decl->bases())
      out->record_bases.push_back(makeCXXBaseSpecifier(base, c));
  out->record_definition = wireId(clava::getId(decl->getDefinition(), c.id));
  return out;
}

std::unique_ptr<obj::ClassTemplateSpecializationDeclDataT>
makeClassTemplateSpecializationDeclData(
    const clang::ClassTemplateSpecializationDecl *decl, Context &c) {
  auto out = declBase<obj::ClassTemplateSpecializationDeclDataT>(decl, c);
  out->base = makeCXXRecordDeclData(decl, c);
  out->specialized_template =
      wireId(clava::getId(decl->getSpecializedTemplate(), c.id));
  out->specialization_kind = enumValue<obj::TemplateSpecializationKind>(
      clava::TEMPLATE_SPECIALIZATION_KIND[decl->getSpecializationKind()]);
  for (const auto &arg : decl->getTemplateArgs().asArray())
    out->template_arguments.push_back(makeTemplateArgument(arg, c));
  return out;
}

std::unique_ptr<obj::NonTypeTemplateParmDeclDataT>
makeNonTypeTemplateParmDeclData(
    const clang::NonTypeTemplateParmDecl *decl, Context &c) {
  auto out = declBase<obj::NonTypeTemplateParmDeclDataT>(decl, c);
  out->base = makeDeclaratorDeclData(decl, c);
  out->default_argument = decl->hasDefaultArgument() ? wireId(clava::getId(decl->getDefaultArgument(), c.id)) : -3;
  out->default_argument_was_inherited = decl->defaultArgumentWasInherited();
  out->is_parameter_pack = decl->isParameterPack();
  out->is_pack_expansion = decl->isPackExpansion();
  out->is_expanded_parameter_pack = decl->isExpandedParameterPack();
  if (decl->isExpandedParameterPack())
    for (unsigned i = 0; i < decl->getNumExpansionTypes(); ++i)
      out->expansion_types.push_back(wireId(clava::getId(
          decl->getExpansionType(i), c.id)));
  return out;
}

std::unique_ptr<obj::TypedefNameDeclDataT> makeTypedefNameDeclData(
    const clang::TypedefNameDecl *decl, Context &c) {
  auto out = declBase<obj::TypedefNameDeclDataT>(decl, c);
  out->base = makeTypeDeclData(decl, c);
  out->underlying_type = wireId(clava::getId(decl->getUnderlyingType(), c.id));
  return out;
}

std::unique_ptr<obj::AccessSpecDeclDataT> makeAccessSpecDeclData(
    const clang::AccessSpecDecl *decl, Context &c) {
  auto out = declBase<obj::AccessSpecDeclDataT>(decl, c);
  out->base = makeDeclData(decl, c);
  out->access_specifier =
      enumValue<obj::AccessSpecifier>(clava::ACCESS_SPECIFIER[decl->getAccess()]);
  return out;
}

std::unique_ptr<obj::UsingDeclDataT> makeUsingDeclData(
    const clang::UsingDecl *decl, Context &c) {
  auto out = declBase<obj::UsingDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->nested_name_specifier = makeNestedNameSpecifier(decl->getQualifier(), c);
  return out;
}

std::unique_ptr<obj::UsingDirectiveDeclDataT> makeUsingDirectiveDeclData(
    const clang::UsingDirectiveDecl *decl, Context &c) {
  auto out = declBase<obj::UsingDirectiveDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->qualifier = sourceText(decl->getSourceRange(), c);
  out->namespace_ = wireId(clava::getId(decl->getNominatedNamespace(), c.id));
  out->namespace_as_written =
      wireId(clava::getId(decl->getNominatedNamespaceAsWritten(), c.id));
  return out;
}

std::unique_ptr<obj::NamespaceDeclDataT> makeNamespaceDeclData(
    const clang::NamespaceDecl *decl, Context &c) {
  auto out = declBase<obj::NamespaceDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->source_literal = sourceText(decl->getSourceRange(), c);
  return out;
}

std::unique_ptr<obj::NamespaceAliasDeclDataT> makeNamespaceAliasDeclData(
    const clang::NamespaceAliasDecl *decl, Context &c) {
  auto out = declBase<obj::NamespaceAliasDeclDataT>(decl, c);
  out->base = makeNamedDeclData(decl, c);
  out->nested_prefix = sourceText(decl->getQualifierLoc().getSourceRange(), c);
  out->aliased_namespace =
      wireId(clava::getId(decl->getAliasedNamespace(), c.id));
  return out;
}

std::unique_ptr<obj::LinkageSpecDeclDataT> makeLinkageSpecDeclData(
    const clang::LinkageSpecDecl *decl, Context &c) {
  auto out = declBase<obj::LinkageSpecDeclDataT>(decl, c);
  out->base = makeDeclData(decl, c);
  switch (decl->getLanguage()) {
  case clang::LinkageSpecLanguageIDs::C:
    out->linkage_type = enumValue<obj::LanguageId>("C");
    break;
  case clang::LinkageSpecLanguageIDs::CXX:
    out->linkage_type = enumValue<obj::LanguageId>("CXX");
    break;
  }
  return out;
}

std::unique_ptr<obj::StaticAssertDeclDataT> makeStaticAssertDeclData(
    const clang::StaticAssertDecl *decl, Context &c) {
  auto out = declBase<obj::StaticAssertDeclDataT>(decl, c);
  out->base = makeDeclData(decl, c);
  out->is_failed = decl->isFailed();
  return out;
}

std::unique_ptr<obj::TemplateTemplateParmDeclDataT>
makeTemplateTemplateParmDeclData(
    const clang::TemplateTemplateParmDecl *decl, Context &c) {
  auto out = declBase<obj::TemplateTemplateParmDeclDataT>(decl, c);
  out->base = makeTemplateDeclData(decl, c);
  if (decl->hasDefaultArgument())
    out->default_argument =
        makeTemplateArgument(decl->getDefaultArgument().getArgument(), c);
  out->is_parameter_pack = decl->isParameterPack();
  out->is_pack_expansion = decl->isPackExpansion();
  out->is_expanded_parameter_pack = decl->isExpandedParameterPack();
  return out;
}

std::unique_ptr<obj::MSPropertyDeclDataT> makeMSPropertyDeclData(
    const clang::MSPropertyDecl *decl, Context &c) {
  auto out = declBase<obj::MSPropertyDeclDataT>(decl, c);
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
std::unique_ptr<obj::CXXMethodDeclDataT> makeCXXDestructorDeclData(
    const clang::CXXDestructorDecl *decl, Context &c) {
  return makeCXXMethodDeclData(decl, c);
}

std::unique_ptr<obj::NamedDeclDataT> makeObjCImplementationDeclData(
    const clang::ObjCImplementationDecl *decl, Context &c) {
  return makeNamedDeclData(decl, c);
}

std::unique_ptr<obj::TemplateDeclDataT> makeClassTemplateDeclData(
    const clang::ClassTemplateDecl *decl, Context &c) {
  return makeTemplateDeclData(decl, c);
}
std::unique_ptr<obj::TemplateDeclDataT> makeFunctionTemplateDeclData(
    const clang::FunctionTemplateDecl *decl, Context &c) {
  return makeTemplateDeclData(decl, c);
}
std::unique_ptr<obj::TemplateDeclDataT> makeTypeAliasTemplateDeclData(
    const clang::TypeAliasTemplateDecl *decl, Context &c) {
  return makeTemplateDeclData(decl, c);
}
std::unique_ptr<obj::TemplateDeclDataT> makeVarTemplateDeclData(
    const clang::VarTemplateDecl *decl, Context &c) {
  return makeTemplateDeclData(decl, c);
}
std::unique_ptr<obj::ValueDeclDataT> makeEnumConstantDeclData(
    const clang::EnumConstantDecl *decl, Context &c) {
  return makeValueDeclData(decl, c);
}
std::unique_ptr<obj::NamedDeclDataT> makeUsingShadowDeclData(
    const clang::UsingShadowDecl *decl, Context &c) {
  return makeNamedDeclData(decl, c);
}
std::unique_ptr<obj::TypedefNameDeclDataT> makeTypeAliasDeclData(
    const clang::TypeAliasDecl *decl, Context &c) {
  return makeTypedefNameDeclData(decl, c);
}
std::unique_ptr<obj::TypedefNameDeclDataT> makeTypedefDeclData(
    const clang::TypedefDecl *decl, Context &c) {
  return makeTypedefNameDeclData(decl, c);
}
std::unique_ptr<obj::NamedDeclDataT> makeLabelDeclData(
    const clang::LabelDecl *decl, Context &c) {
  return makeNamedDeclData(decl, c);
}
std::unique_ptr<obj::VarDeclDataT> makeVarTemplateSpecializationDeclData(
    const clang::VarTemplateSpecializationDecl *decl, Context &c) {
  return makeVarDeclData(decl, c);
}

std::unique_ptr<obj::ClassTemplatePartialSpecializationDeclDataT>
makeClassTemplatePartialSpecializationDeclData(
    const clang::ClassTemplatePartialSpecializationDecl *decl, Context &c) {
  auto out = std::make_unique<obj::ClassTemplatePartialSpecializationDeclDataT>();
  out->base = makeClassTemplateSpecializationDeclData(decl, c);
  return out;
}

} // namespace clava::proto
