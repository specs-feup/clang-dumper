#include "FlatSupport.h"

#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"

#include <llvm/ADT/STLForwardCompat.h>

namespace clava::flat {

namespace {

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
  auto out = std::make_unique<fb::DeclDataT>();
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

namespace {

std::vector<int64_t> templateParameters(const clang::TemplateParameterList *parameters, Context &c) {
  std::vector<int64_t> out;
  if (parameters)
    for (const auto *parameter : *parameters)
      out.push_back(clava::getId(parameter, c.id));
  return out;
}

int64_t primaryTemplateDecl(const clang::FunctionDecl *node, Context &c) {
  auto *primary = node->getPrimaryTemplate();
  return primary ? clava::getId(primary->getTemplatedDecl(), c.id) : -2;
}

std::vector<std::unique_ptr<fb::TemplateArgumentT>> specializationArguments(const clang::FunctionDecl *node,
                                                                          Context &c) {
  std::vector<std::unique_ptr<fb::TemplateArgumentT>> out;
  if (auto *args = node->getTemplateSpecializationArgs())
    for (const auto &arg : args->asArray())
      out.push_back(makeTemplateArgument(arg, c));
  return out;
}

/** The parameters of every template parameter list of the declaration, flattened. */
std::vector<int64_t> functionTemplateParameters(const clang::FunctionDecl *node, Context &c) {
  std::vector<int64_t> out;
  for (unsigned i = 0; i < node->getNumTemplateParameterLists(); ++i)
    for (const auto *parameter : *node->getTemplateParameterList(i))
      out.push_back(clava::getId(parameter, c.id));
  return out;
}

std::vector<int32_t> templateParameterListSizes(const clang::FunctionDecl *node) {
  std::vector<int32_t> out;
  for (unsigned i = 0; i < node->getNumTemplateParameterLists(); ++i)
    out.push_back(node->getTemplateParameterList(i)->size());
  return out;
}

fb::TemplateTypeParmKind templateTypeParmKind(const clang::TemplateTypeParmDecl *node) {
  return enumValue<fb::TemplateTypeParmKind>(node->wasDeclaredWithTypename() ? "TYPENAME" : "CLASS");
}

fb::EnumScopeType enumScopeKind(const clang::EnumDecl *node) {
  if (!node->isScoped())
    return enumValue<fb::EnumScopeType>("NO_SCOPE");
  return enumValue<fb::EnumScopeType>(node->isScopedUsingClassTag() ? "CLASS" : "STRUCT");
}

std::vector<std::unique_ptr<fb::CXXBaseSpecifierT>> recordBases(const clang::CXXRecordDecl *node, Context &c) {
  std::vector<std::unique_ptr<fb::CXXBaseSpecifierT>> out;
  if (node->hasDefinition())
    for (const auto &base : node->bases())
      out.push_back(makeCXXBaseSpecifier(base, c));
  return out;
}

std::vector<int64_t> expansionTypes(const clang::NonTypeTemplateParmDecl *node, Context &c) {
  std::vector<int64_t> out;
  if (node->isExpandedParameterPack())
    for (unsigned i = 0; i < node->getNumExpansionTypes(); ++i)
      out.push_back(clava::getId(node->getExpansionType(i), c.id));
  return out;
}

fb::LanguageId linkageType(const clang::LinkageSpecDecl *node) {
  switch (node->getLanguage()) {
  case clang::LinkageSpecLanguageIDs::C:
    return enumValue<fb::LanguageId>("C");
  case clang::LinkageSpecLanguageIDs::CXX:
    return enumValue<fb::LanguageId>("CXX");
  }
  throw std::invalid_argument("Unsupported linkage specification language");
}

} // namespace

#include "decls_builders.inc"

} // namespace clava::flat
