#pragma once
// Generated from protoc's FileDescriptorSet; do not edit by hand.
#include "clava_ast_wire.pb.h"
#include "ProtoObjects.h"
#include <stdexcept>
#include <string>

namespace clava::proto {
namespace pb = ::astwire::v1;
namespace obj = ::astwire::v1obj;

inline void encode(const obj::RangeT &src, pb::Range *dst);
inline void encode(const obj::SourceInfoT &src, pb::SourceInfo *dst);
inline void encode(const obj::NodeDataT &src, pb::NodeData *dst);
inline void encode(const obj::CXXBaseSpecifierT &src, pb::CXXBaseSpecifier *dst);
inline void encode(const obj::ExplicitSpecifierT &src, pb::ExplicitSpecifier *dst);
inline void encode(const obj::TemplateDeclarationT &src, pb::TemplateDeclaration *dst);
inline void encode(const obj::TemplateNullPtrT &src, pb::TemplateNullPtr *dst);
inline void encode(const obj::TemplateTypeT &src, pb::TemplateType *dst);
inline void encode(const obj::TemplateExpressionT &src, pb::TemplateExpression *dst);
inline void encode(const obj::TemplatePackT &src, pb::TemplatePack *dst);
inline void encode(const obj::TemplateIntegralT &src, pb::TemplateIntegral *dst);
inline void encode(const obj::TemplateExpansionT &src, pb::TemplateExpansion *dst);
inline void encode(const obj::TemplateStructuralValueT &src, pb::TemplateStructuralValue *dst);
inline void encode(const obj::TemplateArgumentT &src, pb::TemplateArgument *dst);
inline void encode(const obj::DirectTemplateNameT &src, pb::DirectTemplateName *dst);
inline void encode(const obj::QualifiedTemplateNameT &src, pb::QualifiedTemplateName *dst);
inline void encode(const obj::SubstitutedTemplateNameT &src, pb::SubstitutedTemplateName *dst);
inline void encode(const obj::UsingTemplateNameT &src, pb::UsingTemplateName *dst);
inline void encode(const obj::DependentTemplateNameT &src, pb::DependentTemplateName *dst);
inline void encode(const obj::TemplateNameT &src, pb::TemplateName *dst);
inline void encode(const obj::AnyMemberInitializerT &src, pb::AnyMemberInitializer *dst);
inline void encode(const obj::BaseInitializerT &src, pb::BaseInitializer *dst);
inline void encode(const obj::DelegatingInitializerT &src, pb::DelegatingInitializer *dst);
inline void encode(const obj::CXXCtorInitializerT &src, pb::CXXCtorInitializer *dst);
inline void encode(const obj::NoExceptionDetailsT &src, pb::NoExceptionDetails *dst);
inline void encode(const obj::ComputedExceptionDetailsT &src, pb::ComputedExceptionDetails *dst);
inline void encode(const obj::UnevaluatedExceptionDetailsT &src, pb::UnevaluatedExceptionDetails *dst);
inline void encode(const obj::UninstantiatedExceptionDetailsT &src, pb::UninstantiatedExceptionDetails *dst);
inline void encode(const obj::ExceptionSpecificationT &src, pb::ExceptionSpecification *dst);
inline void encode(const obj::OffsetArrayT &src, pb::OffsetArray *dst);
inline void encode(const obj::OffsetFieldT &src, pb::OffsetField *dst);
inline void encode(const obj::OffsetIdentifierT &src, pb::OffsetIdentifier *dst);
inline void encode(const obj::OffsetBaseT &src, pb::OffsetBase *dst);
inline void encode(const obj::OffsetOfComponentT &src, pb::OffsetOfComponent *dst);
inline void encode(const obj::FieldDesignatorT &src, pb::FieldDesignator *dst);
inline void encode(const obj::ArrayDesignatorT &src, pb::ArrayDesignator *dst);
inline void encode(const obj::ArrayRangeDesignatorT &src, pb::ArrayRangeDesignator *dst);
inline void encode(const obj::DesignatorT &src, pb::Designator *dst);
inline void encode(const obj::AsmInputT &src, pb::AsmInput *dst);
inline void encode(const obj::AsmOutputT &src, pb::AsmOutput *dst);
inline void encode(const obj::NamespaceSpecifierT &src, pb::NamespaceSpecifier *dst);
inline void encode(const obj::NamespaceAliasSpecifierT &src, pb::NamespaceAliasSpecifier *dst);
inline void encode(const obj::TypeSpecifierT &src, pb::TypeSpecifier *dst);
inline void encode(const obj::TypeWithTemplateSpecifierT &src, pb::TypeWithTemplateSpecifier *dst);
inline void encode(const obj::GlobalSpecifierT &src, pb::GlobalSpecifier *dst);
inline void encode(const obj::SuperSpecifierT &src, pb::SuperSpecifier *dst);
inline void encode(const obj::NestedNameSpecifierT &src, pb::NestedNameSpecifier *dst);
inline void encode(const obj::DeclDataT &src, pb::DeclData *dst);
inline void encode(const obj::NamedDeclDataT &src, pb::NamedDeclData *dst);
inline void encode(const obj::TypeDeclDataT &src, pb::TypeDeclData *dst);
inline void encode(const obj::TagDeclDataT &src, pb::TagDeclData *dst);
inline void encode(const obj::RecordDeclDataT &src, pb::RecordDeclData *dst);
inline void encode(const obj::ValueDeclDataT &src, pb::ValueDeclData *dst);
inline void encode(const obj::DeclaratorDeclDataT &src, pb::DeclaratorDeclData *dst);
inline void encode(const obj::TemplateDeclDataT &src, pb::TemplateDeclData *dst);
inline void encode(const obj::FunctionDeclDataT &src, pb::FunctionDeclData *dst);
inline void encode(const obj::CXXMethodDeclDataT &src, pb::CXXMethodDeclData *dst);
inline void encode(const obj::CXXConstructorDeclDataT &src, pb::CXXConstructorDeclData *dst);
inline void encode(const obj::CXXConversionDeclDataT &src, pb::CXXConversionDeclData *dst);
inline void encode(const obj::FieldDeclDataT &src, pb::FieldDeclData *dst);
inline void encode(const obj::ParmVarDeclDataT &src, pb::ParmVarDeclData *dst);
inline void encode(const obj::VarDeclDataT &src, pb::VarDeclData *dst);
inline void encode(const obj::TemplateTypeParmDeclDataT &src, pb::TemplateTypeParmDeclData *dst);
inline void encode(const obj::UnresolvedUsingTypenameDeclDataT &src, pb::UnresolvedUsingTypenameDeclData *dst);
inline void encode(const obj::EnumDeclDataT &src, pb::EnumDeclData *dst);
inline void encode(const obj::CXXRecordDeclDataT &src, pb::CXXRecordDeclData *dst);
inline void encode(const obj::ClassTemplateSpecializationDeclDataT &src, pb::ClassTemplateSpecializationDeclData *dst);
inline void encode(const obj::NonTypeTemplateParmDeclDataT &src, pb::NonTypeTemplateParmDeclData *dst);
inline void encode(const obj::TypedefNameDeclDataT &src, pb::TypedefNameDeclData *dst);
inline void encode(const obj::AccessSpecDeclDataT &src, pb::AccessSpecDeclData *dst);
inline void encode(const obj::UsingDeclDataT &src, pb::UsingDeclData *dst);
inline void encode(const obj::UsingDirectiveDeclDataT &src, pb::UsingDirectiveDeclData *dst);
inline void encode(const obj::NamespaceDeclDataT &src, pb::NamespaceDeclData *dst);
inline void encode(const obj::NamespaceAliasDeclDataT &src, pb::NamespaceAliasDeclData *dst);
inline void encode(const obj::LinkageSpecDeclDataT &src, pb::LinkageSpecDeclData *dst);
inline void encode(const obj::StaticAssertDeclDataT &src, pb::StaticAssertDeclData *dst);
inline void encode(const obj::TemplateTemplateParmDeclDataT &src, pb::TemplateTemplateParmDeclData *dst);
inline void encode(const obj::MSPropertyDeclDataT &src, pb::MSPropertyDeclData *dst);
inline void encode(const obj::ClassTemplatePartialSpecializationDeclDataT &src, pb::ClassTemplatePartialSpecializationDeclData *dst);
inline void encode(const obj::TypeDataT &src, pb::TypeData *dst);
inline void encode(const obj::QualTypeDataT &src, pb::QualTypeData *dst);
inline void encode(const obj::BuiltinTypeDataT &src, pb::BuiltinTypeData *dst);
inline void encode(const obj::PointerTypeDataT &src, pb::PointerTypeData *dst);
inline void encode(const obj::FunctionTypeDataT &src, pb::FunctionTypeData *dst);
inline void encode(const obj::FunctionProtoTypeDataT &src, pb::FunctionProtoTypeData *dst);
inline void encode(const obj::ArrayTypeDataT &src, pb::ArrayTypeData *dst);
inline void encode(const obj::ConstantArrayTypeDataT &src, pb::ConstantArrayTypeData *dst);
inline void encode(const obj::VariableArrayTypeDataT &src, pb::VariableArrayTypeData *dst);
inline void encode(const obj::DependentSizedArrayTypeDataT &src, pb::DependentSizedArrayTypeData *dst);
inline void encode(const obj::TagTypeDataT &src, pb::TagTypeData *dst);
inline void encode(const obj::TypeWithKeywordDataT &src, pb::TypeWithKeywordData *dst);
inline void encode(const obj::ElaboratedTypeDataT &src, pb::ElaboratedTypeData *dst);
inline void encode(const obj::TemplateTypeParmTypeDataT &src, pb::TemplateTypeParmTypeData *dst);
inline void encode(const obj::TemplateSpecializationTypeDataT &src, pb::TemplateSpecializationTypeData *dst);
inline void encode(const obj::TypedefTypeDataT &src, pb::TypedefTypeData *dst);
inline void encode(const obj::AdjustedTypeDataT &src, pb::AdjustedTypeData *dst);
inline void encode(const obj::DecayedTypeDataT &src, pb::DecayedTypeData *dst);
inline void encode(const obj::DecltypeTypeDataT &src, pb::DecltypeTypeData *dst);
inline void encode(const obj::AutoTypeDataT &src, pb::AutoTypeData *dst);
inline void encode(const obj::ReferenceTypeDataT &src, pb::ReferenceTypeData *dst);
inline void encode(const obj::TypeOfExprTypeDataT &src, pb::TypeOfExprTypeData *dst);
inline void encode(const obj::PackExpansionTypeDataT &src, pb::PackExpansionTypeData *dst);
inline void encode(const obj::UnaryTransformTypeDataT &src, pb::UnaryTransformTypeData *dst);
inline void encode(const obj::AttributedTypeDataT &src, pb::AttributedTypeData *dst);
inline void encode(const obj::SubstTemplateTypeParmTypeDataT &src, pb::SubstTemplateTypeParmTypeData *dst);
inline void encode(const obj::ComplexTypeDataT &src, pb::ComplexTypeData *dst);
inline void encode(const obj::ExprDataT &src, pb::ExprData *dst);
inline void encode(const obj::CastExprDataT &src, pb::CastExprData *dst);
inline void encode(const obj::LiteralDataT &src, pb::LiteralData *dst);
inline void encode(const obj::CharacterLiteralDataT &src, pb::CharacterLiteralData *dst);
inline void encode(const obj::IntegerLiteralDataT &src, pb::IntegerLiteralData *dst);
inline void encode(const obj::FloatingLiteralDataT &src, pb::FloatingLiteralData *dst);
inline void encode(const obj::StringLiteralDataT &src, pb::StringLiteralData *dst);
inline void encode(const obj::CXXBoolLiteralExprDataT &src, pb::CXXBoolLiteralExprData *dst);
inline void encode(const obj::CompoundLiteralExprDataT &src, pb::CompoundLiteralExprData *dst);
inline void encode(const obj::InitListExprDataT &src, pb::InitListExprData *dst);
inline void encode(const obj::DeclRefExprDataT &src, pb::DeclRefExprData *dst);
inline void encode(const obj::OverloadExprDataT &src, pb::OverloadExprData *dst);
inline void encode(const obj::CXXConstructExprDataT &src, pb::CXXConstructExprData *dst);
inline void encode(const obj::CXXTemporaryObjectExprDataT &src, pb::CXXTemporaryObjectExprData *dst);
inline void encode(const obj::MemberExprDataT &src, pb::MemberExprData *dst);
inline void encode(const obj::MaterializeTemporaryExprDataT &src, pb::MaterializeTemporaryExprData *dst);
inline void encode(const obj::BinaryOperatorDataT &src, pb::BinaryOperatorData *dst);
inline void encode(const obj::UnresolvedMemberExprDataT &src, pb::UnresolvedMemberExprData *dst);
inline void encode(const obj::UnresolvedLookupExprDataT &src, pb::UnresolvedLookupExprData *dst);
inline void encode(const obj::CallExprDataT &src, pb::CallExprData *dst);
inline void encode(const obj::CXXMemberCallExprDataT &src, pb::CXXMemberCallExprData *dst);
inline void encode(const obj::CXXTypeidExprDataT &src, pb::CXXTypeidExprData *dst);
inline void encode(const obj::ExplicitCastExprDataT &src, pb::ExplicitCastExprData *dst);
inline void encode(const obj::CXXNamedCastExprDataT &src, pb::CXXNamedCastExprData *dst);
inline void encode(const obj::CXXDependentScopeMemberExprDataT &src, pb::CXXDependentScopeMemberExprData *dst);
inline void encode(const obj::UnaryOperatorDataT &src, pb::UnaryOperatorData *dst);
inline void encode(const obj::UnaryExprOrTypeTraitExprDataT &src, pb::UnaryExprOrTypeTraitExprData *dst);
inline void encode(const obj::CXXNewExprDataT &src, pb::CXXNewExprData *dst);
inline void encode(const obj::CXXDeleteExprDataT &src, pb::CXXDeleteExprData *dst);
inline void encode(const obj::OffsetOfExprDataT &src, pb::OffsetOfExprData *dst);
inline void encode(const obj::LambdaExprDataT &src, pb::LambdaExprData *dst);
inline void encode(const obj::PredefinedExprDataT &src, pb::PredefinedExprData *dst);
inline void encode(const obj::SizeOfPackExprDataT &src, pb::SizeOfPackExprData *dst);
inline void encode(const obj::ArrayInitLoopExprDataT &src, pb::ArrayInitLoopExprData *dst);
inline void encode(const obj::DesignatedInitExprDataT &src, pb::DesignatedInitExprData *dst);
inline void encode(const obj::DependentScopeDeclRefExprDataT &src, pb::DependentScopeDeclRefExprData *dst);
inline void encode(const obj::CXXNoexceptExprDataT &src, pb::CXXNoexceptExprData *dst);
inline void encode(const obj::CXXPseudoDestructorExprDataT &src, pb::CXXPseudoDestructorExprData *dst);
inline void encode(const obj::PseudoObjectExprDataT &src, pb::PseudoObjectExprData *dst);
inline void encode(const obj::MSPropertyRefExprDataT &src, pb::MSPropertyRefExprData *dst);
inline void encode(const obj::StmtDataT &src, pb::StmtData *dst);
inline void encode(const obj::LabelStmtDataT &src, pb::LabelStmtData *dst);
inline void encode(const obj::GotoStmtDataT &src, pb::GotoStmtData *dst);
inline void encode(const obj::AttributedStmtDataT &src, pb::AttributedStmtData *dst);
inline void encode(const obj::AsmStmtDataT &src, pb::AsmStmtData *dst);
inline void encode(const obj::GCCAsmStmtDataT &src, pb::GCCAsmStmtData *dst);
inline void encode(const obj::MSAsmStmtDataT &src, pb::MSAsmStmtData *dst);
inline void encode(const obj::AttributeDataT &src, pb::AttributeData *dst);
inline void encode(const obj::AlignedAttrDataT &src, pb::AlignedAttrData *dst);
inline void encode(const obj::OpenCLUnrollHintAttrDataT &src, pb::OpenCLUnrollHintAttrData *dst);
inline void encode(const obj::FormatAttrDataT &src, pb::FormatAttrData *dst);
inline void encode(const obj::NonNullAttrDataT &src, pb::NonNullAttrData *dst);
inline void encode(const obj::VisibilityAttrDataT &src, pb::VisibilityAttrData *dst);
inline void encode(const obj::FileT &src, pb::File *dst);
inline void encode(const obj::ChildrenT &src, pb::Children *dst);
inline void encode(const obj::NodeClassT &src, pb::NodeClass *dst);
inline void encode(const obj::TopLevelT &src, pb::TopLevel *dst);
inline void encode(const obj::IncludeT &src, pb::Include *dst);
inline void encode(const obj::PragmaT &src, pb::Pragma *dst);
inline void encode(const obj::TranslationUnitFileT &src, pb::TranslationUnitFile *dst);
inline void encode(const obj::CounterT &src, pb::Counter *dst);
inline void encode(const obj::LanguageT &src, pb::Language *dst);
inline void encode(const obj::NodeT &src, pb::Node *dst);

inline void encode(const obj::RangeT &src, pb::Range *dst) {
  dst->set_file(src.file);
  dst->set_line(src.line);
  dst->set_column(src.column);
  dst->set_end_file(src.end_file);
  dst->set_end_line(src.end_line);
  dst->set_end_column(src.end_column);
}

inline void encode(const obj::SourceInfoT &src, pb::SourceInfo *dst) {
  if (src.expansion) encode(*src.expansion, dst->mutable_expansion());
  dst->set_is_macro(src.is_macro);
  if (src.spelling) encode(*src.spelling, dst->mutable_spelling());
  dst->set_system_header(src.system_header);
}

inline void encode(const obj::NodeDataT &src, pb::NodeData *dst) {
  if (src.source) encode(*src.source, dst->mutable_source());
}

inline void encode(const obj::CXXBaseSpecifierT &src, pb::CXXBaseSpecifier *dst) {
  dst->set_is_virtual(src.is_virtual);
  dst->set_is_pack_expansion(src.is_pack_expansion);
  dst->set_access_specifier_as_written(static_cast<pb::AccessSpecifier>(static_cast<int>(src.access_specifier_as_written) + 1));
  dst->set_access_specifier_semantic(static_cast<pb::AccessSpecifier>(static_cast<int>(src.access_specifier_semantic) + 1));
  dst->set_type(src.type);
}

inline void encode(const obj::ExplicitSpecifierT &src, pb::ExplicitSpecifier *dst) {
  dst->set_kind(static_cast<pb::ExplicitSpecKind>(static_cast<int>(src.kind) + 1));
  dst->set_expr(src.expr);
  dst->set_is_specified(src.is_specified);
}

inline void encode(const obj::TemplateDeclarationT &src, pb::TemplateDeclaration *dst) {
  dst->set_decl(src.decl);
}

inline void encode(const obj::TemplateNullPtrT &src, pb::TemplateNullPtr *dst) {
  dst->set_type(src.type);
}

inline void encode(const obj::TemplateTypeT &src, pb::TemplateType *dst) {
  dst->set_type(src.type);
}

inline void encode(const obj::TemplateExpressionT &src, pb::TemplateExpression *dst) {
  dst->set_expr(src.expr);
}

inline void encode(const obj::TemplatePackT &src, pb::TemplatePack *dst) {
  for (const auto &value : src.arguments) {
    if (value) encode(*value, dst->add_arguments());
  }
}

inline void encode(const obj::TemplateIntegralT &src, pb::TemplateIntegral *dst) {
  dst->set_integral(src.integral);
}

inline void encode(const obj::TemplateExpansionT &src, pb::TemplateExpansion *dst) {
  dst->set_num_expansions(src.num_expansions);
  if (src.template_name) encode(*src.template_name, dst->mutable_template_name());
}

inline void encode(const obj::TemplateStructuralValueT &src, pb::TemplateStructuralValue *dst) {
  dst->set_type(src.type);
}

inline void encode(const obj::TemplateArgumentT &src, pb::TemplateArgument *dst) {
  if (const auto *value = src.value.get<obj::TemplateDeclarationT>()) encode(*value, dst->mutable_template_declaration());
  if (const auto *value = src.value.get<obj::TemplateNullPtrT>()) encode(*value, dst->mutable_template_null_ptr());
  if (const auto *value = src.value.get<obj::TemplateTypeT>()) encode(*value, dst->mutable_template_type());
  if (const auto *value = src.value.get<obj::TemplateExpressionT>()) encode(*value, dst->mutable_template_expression());
  if (const auto *value = src.value.get<obj::TemplatePackT>()) encode(*value, dst->mutable_template_pack());
  if (const auto *value = src.value.get<obj::TemplateIntegralT>()) encode(*value, dst->mutable_template_integral());
  if (const auto *value = src.value.get<obj::TemplateNameT>()) encode(*value, dst->mutable_template_name());
  if (const auto *value = src.value.get<obj::TemplateExpansionT>()) encode(*value, dst->mutable_template_expansion());
  if (const auto *value = src.value.get<obj::TemplateStructuralValueT>()) encode(*value, dst->mutable_template_structural_value());
}

inline void encode(const obj::DirectTemplateNameT &src, pb::DirectTemplateName *dst) {
  dst->set_template_decl(src.template_decl);
}

inline void encode(const obj::QualifiedTemplateNameT &src, pb::QualifiedTemplateName *dst) {
  dst->set_qualifier(src.qualifier);
  dst->set_has_template_keyword(src.has_template_keyword);
  dst->set_template_decl(src.template_decl);
}

inline void encode(const obj::SubstitutedTemplateNameT &src, pb::SubstitutedTemplateName *dst) {
  dst->set_parameter(src.parameter);
  if (src.replacement) encode(*src.replacement, dst->mutable_replacement());
}

inline void encode(const obj::UsingTemplateNameT &src, pb::UsingTemplateName *dst) {
  dst->set_using_shadow_decl(src.using_shadow_decl);
}

inline void encode(const obj::DependentTemplateNameT &src, pb::DependentTemplateName *dst) {
  dst->set_qualifier(src.qualifier);
  dst->set_name(src.name);
}

inline void encode(const obj::TemplateNameT &src, pb::TemplateName *dst) {
  if (const auto *value = src.value.get<obj::DirectTemplateNameT>()) encode(*value, dst->mutable_direct_template_name());
  if (const auto *value = src.value.get<obj::QualifiedTemplateNameT>()) encode(*value, dst->mutable_qualified_template_name());
  if (const auto *value = src.value.get<obj::SubstitutedTemplateNameT>()) encode(*value, dst->mutable_substituted_template_name());
  if (const auto *value = src.value.get<obj::UsingTemplateNameT>()) encode(*value, dst->mutable_using_template_name());
  if (const auto *value = src.value.get<obj::DependentTemplateNameT>()) encode(*value, dst->mutable_dependent_template_name());
}

inline void encode(const obj::AnyMemberInitializerT &src, pb::AnyMemberInitializer *dst) {
  dst->set_any_member_decl(src.any_member_decl);
}

inline void encode(const obj::BaseInitializerT &src, pb::BaseInitializer *dst) {
  dst->set_base_class(src.base_class);
}

inline void encode(const obj::DelegatingInitializerT &src, pb::DelegatingInitializer *dst) {
  dst->set_delegated_type(src.delegated_type);
}

inline void encode(const obj::CXXCtorInitializerT &src, pb::CXXCtorInitializer *dst) {
  dst->set_init_expr(src.init_expr);
  dst->set_is_in_class_member_initializer(src.is_in_class_member_initializer);
  dst->set_is_written(src.is_written);
  if (const auto *value = src.target.get<obj::AnyMemberInitializerT>()) encode(*value, dst->mutable_any_member_initializer());
  if (const auto *value = src.target.get<obj::BaseInitializerT>()) encode(*value, dst->mutable_base_initializer());
  if (const auto *value = src.target.get<obj::DelegatingInitializerT>()) encode(*value, dst->mutable_delegating_initializer());
}

inline void encode(const obj::NoExceptionDetailsT &src, pb::NoExceptionDetails *dst) {
}

inline void encode(const obj::ComputedExceptionDetailsT &src, pb::ComputedExceptionDetails *dst) {
  dst->set_noexcept_expr(src.noexcept_expr);
}

inline void encode(const obj::UnevaluatedExceptionDetailsT &src, pb::UnevaluatedExceptionDetails *dst) {
  dst->set_source_decl(src.source_decl);
}

inline void encode(const obj::UninstantiatedExceptionDetailsT &src, pb::UninstantiatedExceptionDetails *dst) {
  dst->set_source_decl(src.source_decl);
  dst->set_source_template(src.source_template);
}

inline void encode(const obj::ExceptionSpecificationT &src, pb::ExceptionSpecification *dst) {
  dst->set_kind(static_cast<pb::ExceptionSpecificationType>(static_cast<int>(src.kind) + 1));
  for (const auto &value : src.exception_types) {
    dst->add_exception_types(value);
  }
  if (const auto *value = src.details.get<obj::NoExceptionDetailsT>()) encode(*value, dst->mutable_no_exception_details());
  if (const auto *value = src.details.get<obj::ComputedExceptionDetailsT>()) encode(*value, dst->mutable_computed_exception_details());
  if (const auto *value = src.details.get<obj::UnevaluatedExceptionDetailsT>()) encode(*value, dst->mutable_unevaluated_exception_details());
  if (const auto *value = src.details.get<obj::UninstantiatedExceptionDetailsT>()) encode(*value, dst->mutable_uninstantiated_exception_details());
}

inline void encode(const obj::OffsetArrayT &src, pb::OffsetArray *dst) {
  dst->set_expr(src.expr);
}

inline void encode(const obj::OffsetFieldT &src, pb::OffsetField *dst) {
  dst->set_field_name(src.field_name);
}

inline void encode(const obj::OffsetIdentifierT &src, pb::OffsetIdentifier *dst) {
  dst->set_field_name(src.field_name);
}

inline void encode(const obj::OffsetBaseT &src, pb::OffsetBase *dst) {
  dst->set_type(src.type);
}

inline void encode(const obj::OffsetOfComponentT &src, pb::OffsetOfComponent *dst) {
  if (const auto *value = src.value.get<obj::OffsetArrayT>()) encode(*value, dst->mutable_offset_array());
  if (const auto *value = src.value.get<obj::OffsetFieldT>()) encode(*value, dst->mutable_offset_field());
  if (const auto *value = src.value.get<obj::OffsetIdentifierT>()) encode(*value, dst->mutable_offset_identifier());
  if (const auto *value = src.value.get<obj::OffsetBaseT>()) encode(*value, dst->mutable_offset_base());
}

inline void encode(const obj::FieldDesignatorT &src, pb::FieldDesignator *dst) {
  dst->set_field_name(src.field_name);
}

inline void encode(const obj::ArrayDesignatorT &src, pb::ArrayDesignator *dst) {
  dst->set_index(src.index);
}

inline void encode(const obj::ArrayRangeDesignatorT &src, pb::ArrayRangeDesignator *dst) {
  dst->set_index(src.index);
}

inline void encode(const obj::DesignatorT &src, pb::Designator *dst) {
  if (const auto *value = src.value.get<obj::FieldDesignatorT>()) encode(*value, dst->mutable_field_designator());
  if (const auto *value = src.value.get<obj::ArrayDesignatorT>()) encode(*value, dst->mutable_array_designator());
  if (const auto *value = src.value.get<obj::ArrayRangeDesignatorT>()) encode(*value, dst->mutable_array_range_designator());
}

inline void encode(const obj::AsmInputT &src, pb::AsmInput *dst) {
  dst->set_expr(src.expr);
  dst->set_constraint(src.constraint);
}

inline void encode(const obj::AsmOutputT &src, pb::AsmOutput *dst) {
  dst->set_expr(src.expr);
  dst->set_constraint(src.constraint);
  dst->set_is_plus_constraint(src.is_plus_constraint);
}

inline void encode(const obj::NamespaceSpecifierT &src, pb::NamespaceSpecifier *dst) {
  dst->set_namespace_decl(src.namespace_decl);
}

inline void encode(const obj::NamespaceAliasSpecifierT &src, pb::NamespaceAliasSpecifier *dst) {
  dst->set_namespace_alias(src.namespace_alias);
}

inline void encode(const obj::TypeSpecifierT &src, pb::TypeSpecifier *dst) {
  dst->set_type(src.type);
}

inline void encode(const obj::TypeWithTemplateSpecifierT &src, pb::TypeWithTemplateSpecifier *dst) {
  dst->set_type(src.type);
}

inline void encode(const obj::GlobalSpecifierT &src, pb::GlobalSpecifier *dst) {
}

inline void encode(const obj::SuperSpecifierT &src, pb::SuperSpecifier *dst) {
  dst->set_super_decl(src.super_decl);
}

inline void encode(const obj::NestedNameSpecifierT &src, pb::NestedNameSpecifier *dst) {
  if (const auto *value = src.value.get<obj::NamespaceSpecifierT>()) encode(*value, dst->mutable_namespace_specifier());
  if (const auto *value = src.value.get<obj::NamespaceAliasSpecifierT>()) encode(*value, dst->mutable_namespace_alias_specifier());
  if (const auto *value = src.value.get<obj::TypeSpecifierT>()) encode(*value, dst->mutable_type_specifier());
  if (const auto *value = src.value.get<obj::TypeWithTemplateSpecifierT>()) encode(*value, dst->mutable_type_with_template_specifier());
  if (const auto *value = src.value.get<obj::GlobalSpecifierT>()) encode(*value, dst->mutable_global_specifier());
  if (const auto *value = src.value.get<obj::SuperSpecifierT>()) encode(*value, dst->mutable_super_specifier());
}

inline void encode(const obj::DeclDataT &src, pb::DeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_implicit(src.is_implicit);
  dst->set_is_used(src.is_used);
  dst->set_is_referenced(src.is_referenced);
  dst->set_is_invalid_decl(src.is_invalid_decl);
  dst->set_is_module_private(src.is_module_private);
  for (const auto &value : src.attributes) {
    dst->add_attributes(value);
  }
}

inline void encode(const obj::NamedDeclDataT &src, pb::NamedDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_qualified_prefix(src.qualified_prefix);
  dst->set_decl_name(src.decl_name);
  dst->set_name_kind(static_cast<pb::NameKind>(static_cast<int>(src.name_kind) + 1));
  dst->set_is_cxx_class_member(src.is_cxx_class_member);
  dst->set_is_cxx_instance_member(src.is_cxx_instance_member);
  dst->set_linkage(static_cast<pb::Linkage>(static_cast<int>(src.linkage) + 1));
  dst->set_visibility(static_cast<pb::Visibility>(static_cast<int>(src.visibility) + 1));
}

inline void encode(const obj::TypeDeclDataT &src, pb::TypeDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_type_for_decl(src.type_for_decl);
}

inline void encode(const obj::TagDeclDataT &src, pb::TagDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_tag_kind(static_cast<pb::TagKind>(static_cast<int>(src.tag_kind) + 1));
  dst->set_is_complete_definition(src.is_complete_definition);
}

inline void encode(const obj::RecordDeclDataT &src, pb::RecordDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_anonymous(src.is_anonymous);
}

inline void encode(const obj::ValueDeclDataT &src, pb::ValueDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_type(src.type);
  dst->set_is_weak(src.is_weak);
}

inline void encode(const obj::DeclaratorDeclDataT &src, pb::DeclaratorDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
}

inline void encode(const obj::TemplateDeclDataT &src, pb::TemplateDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  for (const auto &value : src.template_parameters) {
    dst->add_template_parameters(value);
  }
  dst->set_templated_decl(src.templated_decl);
}

inline void encode(const obj::FunctionDeclDataT &src, pb::FunctionDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_constexpr(src.is_constexpr);
  dst->set_template_kind(static_cast<pb::TemplateKind>(static_cast<int>(src.template_kind) + 1));
  dst->set_storage_class(static_cast<pb::StorageClass>(static_cast<int>(src.storage_class) + 1));
  dst->set_is_inline_specified(src.is_inline_specified);
  dst->set_is_virtual_as_written(src.is_virtual_as_written);
  dst->set_is_pure(src.is_pure);
  dst->set_is_deleted(src.is_deleted);
  dst->set_is_explicitly_defaulted(src.is_explicitly_defaulted);
  dst->set_previous_decl(src.previous_decl);
  dst->set_canonical_decl(src.canonical_decl);
  dst->set_primary_template_decl(src.primary_template_decl);
  for (const auto &value : src.template_arguments) {
    if (value) encode(*value, dst->add_template_arguments());
  }
}

inline void encode(const obj::CXXMethodDeclDataT &src, pb::CXXMethodDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_record(src.record);
  for (const auto &value : src.overridden_methods) {
    dst->add_overridden_methods(value);
  }
  dst->set_is_static(src.is_static);
  dst->set_is_instance(src.is_instance);
  dst->set_is_const(src.is_const);
  dst->set_is_volatile(src.is_volatile);
  dst->set_is_virtual(src.is_virtual);
  dst->set_is_copy_assignment_operator(src.is_copy_assignment_operator);
  dst->set_is_move_assignment_operator(src.is_move_assignment_operator);
  dst->set_this_type(src.this_type);
  dst->set_this_object_type(src.this_object_type);
  dst->set_has_inline_body(src.has_inline_body);
  dst->set_is_lambda_static_invoker(src.is_lambda_static_invoker);
}

inline void encode(const obj::CXXConstructorDeclDataT &src, pb::CXXConstructorDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  for (const auto &value : src.constructor_inits) {
    if (value) encode(*value, dst->add_constructor_inits());
  }
  dst->set_is_default_constructor(src.is_default_constructor);
  dst->set_is_explicit(src.is_explicit);
  if (src.explicit_specifier) encode(*src.explicit_specifier, dst->mutable_explicit_specifier());
}

inline void encode(const obj::CXXConversionDeclDataT &src, pb::CXXConversionDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_explicit(src.is_explicit);
  dst->set_is_lambda_to_block_pointer_conversion(src.is_lambda_to_block_pointer_conversion);
  dst->set_conversion_type(src.conversion_type);
}

inline void encode(const obj::FieldDeclDataT &src, pb::FieldDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_mutable(src.is_mutable);
}

inline void encode(const obj::ParmVarDeclDataT &src, pb::ParmVarDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_has_inherited_default_arg(src.has_inherited_default_arg);
}

inline void encode(const obj::VarDeclDataT &src, pb::VarDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_storage_class(static_cast<pb::StorageClass>(static_cast<int>(src.storage_class) + 1));
  dst->set_tls_kind(static_cast<pb::TLSKind>(static_cast<int>(src.tls_kind) + 1));
  dst->set_is_nrvo_variable(src.is_nrvo_variable);
  dst->set_init_style(static_cast<pb::InitializationStyle>(static_cast<int>(src.init_style) + 1));
  dst->set_is_constexpr(src.is_constexpr);
  dst->set_is_static_data_member(src.is_static_data_member);
  dst->set_is_out_of_line(src.is_out_of_line);
  dst->set_has_global_storage(src.has_global_storage);
}

inline void encode(const obj::TemplateTypeParmDeclDataT &src, pb::TemplateTypeParmDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_kind(static_cast<pb::TemplateTypeParmKind>(static_cast<int>(src.kind) + 1));
  dst->set_is_parameter_pack(src.is_parameter_pack);
  dst->set_default_argument(src.default_argument);
}

inline void encode(const obj::UnresolvedUsingTypenameDeclDataT &src, pb::UnresolvedUsingTypenameDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_qualifier(src.qualifier);
  dst->set_is_pack_expansion(src.is_pack_expansion);
}

inline void encode(const obj::EnumDeclDataT &src, pb::EnumDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_enum_scope_kind(static_cast<pb::EnumScopeType>(static_cast<int>(src.enum_scope_kind) + 1));
  dst->set_integer_type(src.integer_type);
}

inline void encode(const obj::CXXRecordDeclDataT &src, pb::CXXRecordDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  for (const auto &value : src.record_bases) {
    if (value) encode(*value, dst->add_record_bases());
  }
  dst->set_record_definition(src.record_definition);
}

inline void encode(const obj::ClassTemplateSpecializationDeclDataT &src, pb::ClassTemplateSpecializationDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_specialized_template(src.specialized_template);
  dst->set_specialization_kind(static_cast<pb::TemplateSpecializationKind>(static_cast<int>(src.specialization_kind) + 1));
  for (const auto &value : src.template_arguments) {
    if (value) encode(*value, dst->add_template_arguments());
  }
}

inline void encode(const obj::NonTypeTemplateParmDeclDataT &src, pb::NonTypeTemplateParmDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_default_argument(src.default_argument);
  dst->set_default_argument_was_inherited(src.default_argument_was_inherited);
  dst->set_is_parameter_pack(src.is_parameter_pack);
  dst->set_is_pack_expansion(src.is_pack_expansion);
  dst->set_is_expanded_parameter_pack(src.is_expanded_parameter_pack);
  for (const auto &value : src.expansion_types) {
    dst->add_expansion_types(value);
  }
}

inline void encode(const obj::TypedefNameDeclDataT &src, pb::TypedefNameDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_underlying_type(src.underlying_type);
}

inline void encode(const obj::AccessSpecDeclDataT &src, pb::AccessSpecDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_access_specifier(static_cast<pb::AccessSpecifier>(static_cast<int>(src.access_specifier) + 1));
}

inline void encode(const obj::UsingDeclDataT &src, pb::UsingDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  if (src.nested_name_specifier) encode(*src.nested_name_specifier, dst->mutable_nested_name_specifier());
}

inline void encode(const obj::UsingDirectiveDeclDataT &src, pb::UsingDirectiveDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_qualifier(src.qualifier);
  dst->set_namespace_(src.namespace_);
  dst->set_namespace_as_written(src.namespace_as_written);
}

inline void encode(const obj::NamespaceDeclDataT &src, pb::NamespaceDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_source_literal(src.source_literal);
}

inline void encode(const obj::NamespaceAliasDeclDataT &src, pb::NamespaceAliasDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_nested_prefix(src.nested_prefix);
  dst->set_aliased_namespace(src.aliased_namespace);
}

inline void encode(const obj::LinkageSpecDeclDataT &src, pb::LinkageSpecDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_linkage_type(static_cast<pb::LanguageId>(static_cast<int>(src.linkage_type) + 1));
}

inline void encode(const obj::StaticAssertDeclDataT &src, pb::StaticAssertDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_failed(src.is_failed);
}

inline void encode(const obj::TemplateTemplateParmDeclDataT &src, pb::TemplateTemplateParmDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  if (src.default_argument) encode(*src.default_argument, dst->mutable_default_argument());
  dst->set_is_parameter_pack(src.is_parameter_pack);
  dst->set_is_pack_expansion(src.is_pack_expansion);
  dst->set_is_expanded_parameter_pack(src.is_expanded_parameter_pack);
}

inline void encode(const obj::MSPropertyDeclDataT &src, pb::MSPropertyDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_getter_name(src.getter_name);
  dst->set_setter_name(src.setter_name);
}

inline void encode(const obj::ClassTemplatePartialSpecializationDeclDataT &src, pb::ClassTemplatePartialSpecializationDeclData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
}

inline void encode(const obj::TypeDataT &src, pb::TypeData *dst) {
  dst->set_type_as_string(src.type_as_string);
  dst->set_type_dependency(static_cast<pb::TypeDependency>(static_cast<int>(src.type_dependency) + 1));
  dst->set_is_variably_modified(src.is_variably_modified);
  dst->set_contains_unexpanded_parameter_pack(src.contains_unexpanded_parameter_pack);
  dst->set_is_from_ast(src.is_from_ast);
  dst->set_unqualified_desugared_type(src.unqualified_desugared_type);
}

inline void encode(const obj::QualTypeDataT &src, pb::QualTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  for (const auto &value : src.c99_qualifiers) {
    dst->add_c99_qualifiers(static_cast<pb::C99Qualifier>(static_cast<int>(value) + 1));
  }
  dst->set_address_space_qualifier(static_cast<pb::AddressSpaceQualifierV2>(static_cast<int>(src.address_space_qualifier) + 1));
  dst->set_address_space(src.address_space);
  dst->set_unqualified_type(src.unqualified_type);
}

inline void encode(const obj::BuiltinTypeDataT &src, pb::BuiltinTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_kind(static_cast<pb::BuiltinKind>(static_cast<int>(src.kind) + 1));
  dst->set_kind_literal(src.kind_literal);
}

inline void encode(const obj::PointerTypeDataT &src, pb::PointerTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_pointee_type(src.pointee_type);
}

inline void encode(const obj::FunctionTypeDataT &src, pb::FunctionTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_const(src.is_const);
  dst->set_is_volatile(src.is_volatile);
  dst->set_is_restrict(src.is_restrict);
  dst->set_no_return(src.no_return);
  dst->set_produces_result(src.produces_result);
  dst->set_uses_reg_parm(src.uses_reg_parm);
  dst->set_reg_parm(src.reg_parm);
  dst->set_calling_convention(static_cast<pb::CallingConvention>(static_cast<int>(src.calling_convention) + 1));
  dst->set_return_type(src.return_type);
}

inline void encode(const obj::FunctionProtoTypeDataT &src, pb::FunctionProtoTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_num_parameters(src.num_parameters);
  for (const auto &value : src.parameters_types) {
    dst->add_parameters_types(value);
  }
  dst->set_has_trailing_returns(src.has_trailing_returns);
  dst->set_is_variadic(src.is_variadic);
  dst->set_reference_qualifier(static_cast<pb::ReferenceQualifier>(static_cast<int>(src.reference_qualifier) + 1));
  if (src.exception_specification) encode(*src.exception_specification, dst->mutable_exception_specification());
}

inline void encode(const obj::ArrayTypeDataT &src, pb::ArrayTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_array_size_modifier(static_cast<pb::ArraySizeModifier>(static_cast<int>(src.array_size_modifier) + 1));
  for (const auto &value : src.index_type_qualifiers) {
    dst->add_index_type_qualifiers(static_cast<pb::C99Qualifier>(static_cast<int>(value) + 1));
  }
  dst->set_element_type(src.element_type);
}

inline void encode(const obj::ConstantArrayTypeDataT &src, pb::ConstantArrayTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_array_size(src.array_size);
}

inline void encode(const obj::VariableArrayTypeDataT &src, pb::VariableArrayTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_size_expr(src.size_expr);
}

inline void encode(const obj::DependentSizedArrayTypeDataT &src, pb::DependentSizedArrayTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_size_expr(src.size_expr);
}

inline void encode(const obj::TagTypeDataT &src, pb::TagTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_decl(src.decl);
}

inline void encode(const obj::TypeWithKeywordDataT &src, pb::TypeWithKeywordData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_elaborated_type_keyword(static_cast<pb::ElaboratedTypeKeyword>(static_cast<int>(src.elaborated_type_keyword) + 1));
}

inline void encode(const obj::ElaboratedTypeDataT &src, pb::ElaboratedTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_qualifier(src.qualifier);
  dst->set_named_type(src.named_type);
}

inline void encode(const obj::TemplateTypeParmTypeDataT &src, pb::TemplateTypeParmTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_depth(src.depth);
  dst->set_index(src.index);
  dst->set_is_packed(src.is_packed);
  dst->set_decl(src.decl);
}

inline void encode(const obj::TemplateSpecializationTypeDataT &src, pb::TemplateSpecializationTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_type_alias(src.is_type_alias);
  dst->set_aliased_type(src.aliased_type);
  dst->set_template_name(src.template_name);
  dst->set_template_decl(src.template_decl);
  for (const auto &value : src.template_arguments) {
    if (value) encode(*value, dst->add_template_arguments());
  }
}

inline void encode(const obj::TypedefTypeDataT &src, pb::TypedefTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_decl(src.decl);
}

inline void encode(const obj::AdjustedTypeDataT &src, pb::AdjustedTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_original_type(src.original_type);
  dst->set_adjusted_type(src.adjusted_type);
}

inline void encode(const obj::DecayedTypeDataT &src, pb::DecayedTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_decayed_type(src.decayed_type);
  dst->set_pointee_type(src.pointee_type);
}

inline void encode(const obj::DecltypeTypeDataT &src, pb::DecltypeTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_sugared(src.is_sugared);
  dst->set_underlying_expr(src.underlying_expr);
}

inline void encode(const obj::AutoTypeDataT &src, pb::AutoTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_deduced_type(src.deduced_type);
}

inline void encode(const obj::ReferenceTypeDataT &src, pb::ReferenceTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_pointee_type_as_written(src.pointee_type_as_written);
}

inline void encode(const obj::TypeOfExprTypeDataT &src, pb::TypeOfExprTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_sugared(src.is_sugared);
  dst->set_underlying_expr(src.underlying_expr);
}

inline void encode(const obj::PackExpansionTypeDataT &src, pb::PackExpansionTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_num_expansions(src.num_expansions);
  dst->set_pattern(src.pattern);
}

inline void encode(const obj::UnaryTransformTypeDataT &src, pb::UnaryTransformTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_kind(static_cast<pb::UnaryTransformTypeKind>(static_cast<int>(src.kind) + 1));
  dst->set_underlying_type(src.underlying_type);
  dst->set_base_type(src.base_type);
}

inline void encode(const obj::AttributedTypeDataT &src, pb::AttributedTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_modified_type(src.modified_type);
  dst->set_equivalent_type(src.equivalent_type);
}

inline void encode(const obj::SubstTemplateTypeParmTypeDataT &src, pb::SubstTemplateTypeParmTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_replaced_parameter(src.replaced_parameter);
  dst->set_replacement_type(src.replacement_type);
}

inline void encode(const obj::ComplexTypeDataT &src, pb::ComplexTypeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_element_type(src.element_type);
}

inline void encode(const obj::ExprDataT &src, pb::ExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_type(src.type);
  dst->set_value_kind(static_cast<pb::ValueKind>(static_cast<int>(src.value_kind) + 1));
  dst->set_object_kind(static_cast<pb::ObjectKind>(static_cast<int>(src.object_kind) + 1));
  dst->set_is_default_argument(src.is_default_argument);
}

inline void encode(const obj::CastExprDataT &src, pb::CastExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_cast_kind(static_cast<pb::CastKind>(static_cast<int>(src.cast_kind) + 1));
}

inline void encode(const obj::LiteralDataT &src, pb::LiteralData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_source_literal(src.source_literal);
}

inline void encode(const obj::CharacterLiteralDataT &src, pb::CharacterLiteralData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_value(src.value);
  dst->set_kind(static_cast<pb::CharacterKind>(static_cast<int>(src.kind) + 1));
}

inline void encode(const obj::IntegerLiteralDataT &src, pb::IntegerLiteralData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_value(src.value);
}

inline void encode(const obj::FloatingLiteralDataT &src, pb::FloatingLiteralData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_value(src.value);
}

inline void encode(const obj::StringLiteralDataT &src, pb::StringLiteralData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_string_kind(static_cast<pb::StringKind>(static_cast<int>(src.string_kind) + 1));
  dst->set_length(src.length);
  dst->set_char_byte_width(src.char_byte_width);
  for (const auto &value : src.string_bytes) {
    dst->add_string_bytes(value);
  }
}

inline void encode(const obj::CXXBoolLiteralExprDataT &src, pb::CXXBoolLiteralExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_value(src.value);
}

inline void encode(const obj::CompoundLiteralExprDataT &src, pb::CompoundLiteralExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_file_scope(src.is_file_scope);
}

inline void encode(const obj::InitListExprDataT &src, pb::InitListExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_array_filler(src.array_filler);
  dst->set_is_explicit(src.is_explicit);
  dst->set_is_string_literal_init(src.is_string_literal_init);
  dst->set_syntactic_form(src.syntactic_form);
  dst->set_semantic_form(src.semantic_form);
}

inline void encode(const obj::DeclRefExprDataT &src, pb::DeclRefExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_qualifier(src.qualifier);
  for (const auto &value : src.template_arguments) {
    if (value) encode(*value, dst->add_template_arguments());
  }
  dst->set_decl(src.decl);
}

inline void encode(const obj::OverloadExprDataT &src, pb::OverloadExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_qualifier(src.qualifier);
  dst->set_name(src.name);
  for (const auto &value : src.unresolved_decls) {
    dst->add_unresolved_decls(value);
  }
  for (const auto &value : src.template_arguments) {
    if (value) encode(*value, dst->add_template_arguments());
  }
}

inline void encode(const obj::CXXConstructExprDataT &src, pb::CXXConstructExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_elidable(src.is_elidable);
  dst->set_requires_zero_initialization(src.requires_zero_initialization);
  dst->set_is_list_initialization(src.is_list_initialization);
  dst->set_is_std_list_initialization(src.is_std_list_initialization);
  dst->set_construction_kind(static_cast<pb::ConstructionKind>(static_cast<int>(src.construction_kind) + 1));
  dst->set_is_temporary_object(src.is_temporary_object);
  dst->set_constructor_decl(src.constructor_decl);
}

inline void encode(const obj::CXXTemporaryObjectExprDataT &src, pb::CXXTemporaryObjectExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
}

inline void encode(const obj::MemberExprDataT &src, pb::MemberExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_arrow(src.is_arrow);
  dst->set_member_name(src.member_name);
  dst->set_member_decl(src.member_decl);
  dst->set_found_decl(src.found_decl);
  dst->set_found_decl_access_specifier(static_cast<pb::AccessSpecifier>(static_cast<int>(src.found_decl_access_specifier) + 1));
}

inline void encode(const obj::MaterializeTemporaryExprDataT &src, pb::MaterializeTemporaryExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_extending_decl(src.extending_decl);
}

inline void encode(const obj::BinaryOperatorDataT &src, pb::BinaryOperatorData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_op(static_cast<pb::BinaryOperatorKind>(static_cast<int>(src.op) + 1));
}

inline void encode(const obj::UnresolvedMemberExprDataT &src, pb::UnresolvedMemberExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
}

inline void encode(const obj::UnresolvedLookupExprDataT &src, pb::UnresolvedLookupExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_requires_adl(src.requires_adl);
}

inline void encode(const obj::CallExprDataT &src, pb::CallExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_direct_callee(src.direct_callee);
}

inline void encode(const obj::CXXMemberCallExprDataT &src, pb::CXXMemberCallExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_method_decl(src.method_decl);
}

inline void encode(const obj::CXXTypeidExprDataT &src, pb::CXXTypeidExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_type_operand(src.is_type_operand);
  dst->set_operand(src.operand);
}

inline void encode(const obj::ExplicitCastExprDataT &src, pb::ExplicitCastExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_type_as_written(src.type_as_written);
}

inline void encode(const obj::CXXNamedCastExprDataT &src, pb::CXXNamedCastExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_cast_name(src.cast_name);
}

inline void encode(const obj::CXXDependentScopeMemberExprDataT &src, pb::CXXDependentScopeMemberExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_arrow(src.is_arrow);
  dst->set_member_name(src.member_name);
  dst->set_is_implicit_access(src.is_implicit_access);
  dst->set_qualifier(src.qualifier);
  dst->set_has_template_keyword(src.has_template_keyword);
  for (const auto &value : src.template_arguments) {
    if (value) encode(*value, dst->add_template_arguments());
  }
}

inline void encode(const obj::UnaryOperatorDataT &src, pb::UnaryOperatorData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_op(static_cast<pb::UnaryOperatorKind>(static_cast<int>(src.op) + 1));
  dst->set_position(static_cast<pb::UnaryOperatorPosition>(static_cast<int>(src.position) + 1));
}

inline void encode(const obj::UnaryExprOrTypeTraitExprDataT &src, pb::UnaryExprOrTypeTraitExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_kind(static_cast<pb::UnaryExprOrTypeTrait>(static_cast<int>(src.kind) + 1));
  dst->set_is_argument_type(src.is_argument_type);
  dst->set_arg_type(src.arg_type);
  dst->set_source_literal(src.source_literal);
}

inline void encode(const obj::CXXNewExprDataT &src, pb::CXXNewExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_global(src.is_global);
  dst->set_is_array(src.is_array);
  dst->set_initialization_present(src.initialization_present);
  dst->set_init_style(static_cast<pb::NewInitStyle>(static_cast<int>(src.init_style) + 1));
  dst->set_initializer(src.initializer);
  dst->set_construct_expr(src.construct_expr);
  dst->set_array_size(src.array_size);
  dst->set_operator_new(src.operator_new);
}

inline void encode(const obj::CXXDeleteExprDataT &src, pb::CXXDeleteExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_global(src.is_global);
  dst->set_is_array(src.is_array);
  dst->set_is_array_as_written(src.is_array_as_written);
}

inline void encode(const obj::OffsetOfExprDataT &src, pb::OffsetOfExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_source_type(src.source_type);
  for (const auto &value : src.components) {
    if (value) encode(*value, dst->add_components());
  }
}

inline void encode(const obj::LambdaExprDataT &src, pb::LambdaExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_generic_lambda(src.is_generic_lambda);
  dst->set_is_mutable(src.is_mutable);
  dst->set_has_explicit_parameters(src.has_explicit_parameters);
  dst->set_has_explicit_result_type(src.has_explicit_result_type);
  dst->set_capture_default(static_cast<pb::LambdaCaptureDefault>(static_cast<int>(src.capture_default) + 1));
  dst->set_lambda_class(src.lambda_class);
  for (const auto &value : src.capture_kinds) {
    dst->add_capture_kinds(static_cast<pb::LambdaCaptureKind>(static_cast<int>(value) + 1));
  }
}

inline void encode(const obj::PredefinedExprDataT &src, pb::PredefinedExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_predefined_type(static_cast<pb::PredefinedIdType>(static_cast<int>(src.predefined_type) + 1));
}

inline void encode(const obj::SizeOfPackExprDataT &src, pb::SizeOfPackExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_partially_substituted(src.is_partially_substituted);
  dst->set_pack(src.pack);
  for (const auto &value : src.partial_arguments) {
    if (value) encode(*value, dst->add_partial_arguments());
  }
}

inline void encode(const obj::ArrayInitLoopExprDataT &src, pb::ArrayInitLoopExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
}

inline void encode(const obj::DesignatedInitExprDataT &src, pb::DesignatedInitExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_uses_gnu_syntax(src.uses_gnu_syntax);
  for (const auto &value : src.designators) {
    if (value) encode(*value, dst->add_designators());
  }
}

inline void encode(const obj::DependentScopeDeclRefExprDataT &src, pb::DependentScopeDeclRefExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_decl_name(src.decl_name);
  dst->set_qualifier(src.qualifier);
  dst->set_has_template_keyword(src.has_template_keyword);
  for (const auto &value : src.template_arguments) {
    if (value) encode(*value, dst->add_template_arguments());
  }
}

inline void encode(const obj::CXXNoexceptExprDataT &src, pb::CXXNoexceptExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_value(src.value);
}

inline void encode(const obj::CXXPseudoDestructorExprDataT &src, pb::CXXPseudoDestructorExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_qualifier(src.qualifier);
  dst->set_is_arrow(src.is_arrow);
  dst->set_destroyed_type(src.destroyed_type);
}

inline void encode(const obj::PseudoObjectExprDataT &src, pb::PseudoObjectExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_result_expr_index(src.result_expr_index);
}

inline void encode(const obj::MSPropertyRefExprDataT &src, pb::MSPropertyRefExprData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_base_expr(src.base_expr);
  dst->set_property_decl(src.property_decl);
  dst->set_is_implicit_access(src.is_implicit_access);
  dst->set_is_arrow(src.is_arrow);
}

inline void encode(const obj::StmtDataT &src, pb::StmtData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
}

inline void encode(const obj::LabelStmtDataT &src, pb::LabelStmtData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_label(src.label);
}

inline void encode(const obj::GotoStmtDataT &src, pb::GotoStmtData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_label(src.label);
}

inline void encode(const obj::AttributedStmtDataT &src, pb::AttributedStmtData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  for (const auto &value : src.stmt_attributes) {
    dst->add_stmt_attributes(value);
  }
}

inline void encode(const obj::AsmStmtDataT &src, pb::AsmStmtData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_is_simple(src.is_simple);
  dst->set_is_volatile(src.is_volatile);
  for (const auto &value : src.clobbers) {
    dst->add_clobbers(value);
  }
  for (const auto &value : src.outputs) {
    if (value) encode(*value, dst->add_outputs());
  }
  for (const auto &value : src.inputs) {
    if (value) encode(*value, dst->add_inputs());
  }
}

inline void encode(const obj::GCCAsmStmtDataT &src, pb::GCCAsmStmtData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_asm_string(src.asm_string);
}

inline void encode(const obj::MSAsmStmtDataT &src, pb::MSAsmStmtData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_asm_string(src.asm_string);
}

inline void encode(const obj::AttributeDataT &src, pb::AttributeData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_kind(static_cast<pb::AttributeKind>(static_cast<int>(src.kind) + 1));
  dst->set_is_implicit(src.is_implicit);
  dst->set_is_inherited(src.is_inherited);
  dst->set_is_late_parsed(src.is_late_parsed);
  dst->set_is_pack_expansion(src.is_pack_expansion);
}

inline void encode(const obj::AlignedAttrDataT &src, pb::AlignedAttrData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_spelling(src.spelling);
  dst->set_is_expression(src.is_expression);
  dst->set_alignment(src.alignment);
}

inline void encode(const obj::OpenCLUnrollHintAttrDataT &src, pb::OpenCLUnrollHintAttrData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_unroll_hint(src.unroll_hint);
}

inline void encode(const obj::FormatAttrDataT &src, pb::FormatAttrData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_type(static_cast<pb::FormatAttrKind>(static_cast<int>(src.type) + 1));
  dst->set_format_index(src.format_index);
  dst->set_first_arg(src.first_arg);
}

inline void encode(const obj::NonNullAttrDataT &src, pb::NonNullAttrData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  for (const auto &value : src.arguments) {
    dst->add_arguments(value);
  }
}

inline void encode(const obj::VisibilityAttrDataT &src, pb::VisibilityAttrData *dst) {
  if (src.base) encode(*src.base, dst->mutable_base());
  dst->set_visibility_type(static_cast<pb::VisibilityType>(static_cast<int>(src.visibility_type) + 1));
}

inline void encode(const obj::FileT &src, pb::File *dst) {
  dst->set_id(src.id);
  dst->set_path(src.path);
}

inline void encode(const obj::ChildrenT &src, pb::Children *dst) {
  dst->set_node(src.node);
  for (const auto &value : src.children) {
    dst->add_children(value);
  }
}

inline void encode(const obj::NodeClassT &src, pb::NodeClass *dst) {
  dst->set_node(src.node);
  dst->set_class_name(src.class_name);
}

inline void encode(const obj::TopLevelT &src, pb::TopLevel *dst) {
  dst->set_kind(static_cast<pb::TopLevelKind>(static_cast<int>(src.kind) + 1));
  dst->set_node(src.node);
}

inline void encode(const obj::IncludeT &src, pb::Include *dst) {
  dst->set_source(src.source);
  dst->set_name(src.name);
  dst->set_line(src.line);
  dst->set_angled(src.angled);
}

inline void encode(const obj::PragmaT &src, pb::Pragma *dst) {
  dst->set_source(src.source);
  dst->set_line(src.line);
  dst->set_column(src.column);
}

inline void encode(const obj::TranslationUnitFileT &src, pb::TranslationUnitFile *dst) {
  dst->set_id(src.id);
  dst->set_path(src.path);
}

inline void encode(const obj::CounterT &src, pb::Counter *dst) {
  dst->set_value(src.value);
}

inline void encode(const obj::LanguageT &src, pb::Language *dst) {
  dst->set_file(src.file);
  dst->set_line_comment(src.line_comment);
  dst->set_gnu_inline(src.gnu_inline);
  dst->set_c99(src.c99);
  dst->set_c11(src.c11);
  dst->set_c_plus_plus(src.c_plus_plus);
  dst->set_c_plus_plus_11(src.c_plus_plus_11);
  dst->set_c_plus_plus_14(src.c_plus_plus_14);
  dst->set_c_plus_plus_17(src.c_plus_plus_17);
  dst->set_c_plus_plus_20(src.c_plus_plus_20);
  dst->set_c_plus_plus_23(src.c_plus_plus_23);
  dst->set_c_plus_plus_26(src.c_plus_plus_26);
  dst->set_has_digraphs(src.has_digraphs);
  dst->set_is_gnu(src.is_gnu);
  dst->set_hex_floats(src.hex_floats);
  dst->set_open_cl(src.open_cl);
  dst->set_open_cl_version(src.open_cl_version);
  dst->set_native_half_type(src.native_half_type);
  dst->set_cuda(src.cuda);
  dst->set_has_bool(src.has_bool);
  dst->set_has_half(src.has_half);
  dst->set_has_wchar(src.has_wchar);
  dst->set_char_width(src.char_width);
  dst->set_float_width(src.float_width);
  dst->set_double_width(src.double_width);
  dst->set_long_double_width(src.long_double_width);
  dst->set_bool_width(src.bool_width);
  dst->set_short_width(src.short_width);
  dst->set_int_width(src.int_width);
  dst->set_long_width(src.long_width);
  dst->set_long_long_width(src.long_long_width);
}

inline void encode(const obj::NodeT &src, pb::Node *dst) {
  dst->set_id(src.id);
  dst->set_class_name(src.class_name);
  if (const auto *value = src.payload.get<obj::DeclDataT>()) encode(*value, dst->mutable_decl_data());
  if (const auto *value = src.payload.get<obj::NamedDeclDataT>()) encode(*value, dst->mutable_named_decl_data());
  if (const auto *value = src.payload.get<obj::TypeDeclDataT>()) encode(*value, dst->mutable_type_decl_data());
  if (const auto *value = src.payload.get<obj::TagDeclDataT>()) encode(*value, dst->mutable_tag_decl_data());
  if (const auto *value = src.payload.get<obj::RecordDeclDataT>()) encode(*value, dst->mutable_record_decl_data());
  if (const auto *value = src.payload.get<obj::ValueDeclDataT>()) encode(*value, dst->mutable_value_decl_data());
  if (const auto *value = src.payload.get<obj::DeclaratorDeclDataT>()) encode(*value, dst->mutable_declarator_decl_data());
  if (const auto *value = src.payload.get<obj::TemplateDeclDataT>()) encode(*value, dst->mutable_template_decl_data());
  if (const auto *value = src.payload.get<obj::FunctionDeclDataT>()) encode(*value, dst->mutable_function_decl_data());
  if (const auto *value = src.payload.get<obj::CXXMethodDeclDataT>()) encode(*value, dst->mutable_c_x_x_method_decl_data());
  if (const auto *value = src.payload.get<obj::CXXConstructorDeclDataT>()) encode(*value, dst->mutable_c_x_x_constructor_decl_data());
  if (const auto *value = src.payload.get<obj::CXXConversionDeclDataT>()) encode(*value, dst->mutable_c_x_x_conversion_decl_data());
  if (const auto *value = src.payload.get<obj::FieldDeclDataT>()) encode(*value, dst->mutable_field_decl_data());
  if (const auto *value = src.payload.get<obj::ParmVarDeclDataT>()) encode(*value, dst->mutable_parm_var_decl_data());
  if (const auto *value = src.payload.get<obj::VarDeclDataT>()) encode(*value, dst->mutable_var_decl_data());
  if (const auto *value = src.payload.get<obj::TemplateTypeParmDeclDataT>()) encode(*value, dst->mutable_template_type_parm_decl_data());
  if (const auto *value = src.payload.get<obj::UnresolvedUsingTypenameDeclDataT>()) encode(*value, dst->mutable_unresolved_using_typename_decl_data());
  if (const auto *value = src.payload.get<obj::EnumDeclDataT>()) encode(*value, dst->mutable_enum_decl_data());
  if (const auto *value = src.payload.get<obj::CXXRecordDeclDataT>()) encode(*value, dst->mutable_c_x_x_record_decl_data());
  if (const auto *value = src.payload.get<obj::ClassTemplateSpecializationDeclDataT>()) encode(*value, dst->mutable_class_template_specialization_decl_data());
  if (const auto *value = src.payload.get<obj::NonTypeTemplateParmDeclDataT>()) encode(*value, dst->mutable_non_type_template_parm_decl_data());
  if (const auto *value = src.payload.get<obj::TypedefNameDeclDataT>()) encode(*value, dst->mutable_typedef_name_decl_data());
  if (const auto *value = src.payload.get<obj::AccessSpecDeclDataT>()) encode(*value, dst->mutable_access_spec_decl_data());
  if (const auto *value = src.payload.get<obj::UsingDeclDataT>()) encode(*value, dst->mutable_using_decl_data());
  if (const auto *value = src.payload.get<obj::UsingDirectiveDeclDataT>()) encode(*value, dst->mutable_using_directive_decl_data());
  if (const auto *value = src.payload.get<obj::NamespaceDeclDataT>()) encode(*value, dst->mutable_namespace_decl_data());
  if (const auto *value = src.payload.get<obj::NamespaceAliasDeclDataT>()) encode(*value, dst->mutable_namespace_alias_decl_data());
  if (const auto *value = src.payload.get<obj::LinkageSpecDeclDataT>()) encode(*value, dst->mutable_linkage_spec_decl_data());
  if (const auto *value = src.payload.get<obj::StaticAssertDeclDataT>()) encode(*value, dst->mutable_static_assert_decl_data());
  if (const auto *value = src.payload.get<obj::TemplateTemplateParmDeclDataT>()) encode(*value, dst->mutable_template_template_parm_decl_data());
  if (const auto *value = src.payload.get<obj::MSPropertyDeclDataT>()) encode(*value, dst->mutable_m_s_property_decl_data());
  if (const auto *value = src.payload.get<obj::ClassTemplatePartialSpecializationDeclDataT>()) encode(*value, dst->mutable_class_template_partial_specialization_decl_data());
  if (const auto *value = src.payload.get<obj::TypeDataT>()) encode(*value, dst->mutable_type_data());
  if (const auto *value = src.payload.get<obj::QualTypeDataT>()) encode(*value, dst->mutable_qual_type_data());
  if (const auto *value = src.payload.get<obj::BuiltinTypeDataT>()) encode(*value, dst->mutable_builtin_type_data());
  if (const auto *value = src.payload.get<obj::PointerTypeDataT>()) encode(*value, dst->mutable_pointer_type_data());
  if (const auto *value = src.payload.get<obj::FunctionTypeDataT>()) encode(*value, dst->mutable_function_type_data());
  if (const auto *value = src.payload.get<obj::FunctionProtoTypeDataT>()) encode(*value, dst->mutable_function_proto_type_data());
  if (const auto *value = src.payload.get<obj::ArrayTypeDataT>()) encode(*value, dst->mutable_array_type_data());
  if (const auto *value = src.payload.get<obj::ConstantArrayTypeDataT>()) encode(*value, dst->mutable_constant_array_type_data());
  if (const auto *value = src.payload.get<obj::VariableArrayTypeDataT>()) encode(*value, dst->mutable_variable_array_type_data());
  if (const auto *value = src.payload.get<obj::DependentSizedArrayTypeDataT>()) encode(*value, dst->mutable_dependent_sized_array_type_data());
  if (const auto *value = src.payload.get<obj::TagTypeDataT>()) encode(*value, dst->mutable_tag_type_data());
  if (const auto *value = src.payload.get<obj::TypeWithKeywordDataT>()) encode(*value, dst->mutable_type_with_keyword_data());
  if (const auto *value = src.payload.get<obj::ElaboratedTypeDataT>()) encode(*value, dst->mutable_elaborated_type_data());
  if (const auto *value = src.payload.get<obj::TemplateTypeParmTypeDataT>()) encode(*value, dst->mutable_template_type_parm_type_data());
  if (const auto *value = src.payload.get<obj::TemplateSpecializationTypeDataT>()) encode(*value, dst->mutable_template_specialization_type_data());
  if (const auto *value = src.payload.get<obj::TypedefTypeDataT>()) encode(*value, dst->mutable_typedef_type_data());
  if (const auto *value = src.payload.get<obj::AdjustedTypeDataT>()) encode(*value, dst->mutable_adjusted_type_data());
  if (const auto *value = src.payload.get<obj::DecayedTypeDataT>()) encode(*value, dst->mutable_decayed_type_data());
  if (const auto *value = src.payload.get<obj::DecltypeTypeDataT>()) encode(*value, dst->mutable_decltype_type_data());
  if (const auto *value = src.payload.get<obj::AutoTypeDataT>()) encode(*value, dst->mutable_auto_type_data());
  if (const auto *value = src.payload.get<obj::ReferenceTypeDataT>()) encode(*value, dst->mutable_reference_type_data());
  if (const auto *value = src.payload.get<obj::TypeOfExprTypeDataT>()) encode(*value, dst->mutable_type_of_expr_type_data());
  if (const auto *value = src.payload.get<obj::PackExpansionTypeDataT>()) encode(*value, dst->mutable_pack_expansion_type_data());
  if (const auto *value = src.payload.get<obj::UnaryTransformTypeDataT>()) encode(*value, dst->mutable_unary_transform_type_data());
  if (const auto *value = src.payload.get<obj::AttributedTypeDataT>()) encode(*value, dst->mutable_attributed_type_data());
  if (const auto *value = src.payload.get<obj::SubstTemplateTypeParmTypeDataT>()) encode(*value, dst->mutable_subst_template_type_parm_type_data());
  if (const auto *value = src.payload.get<obj::ComplexTypeDataT>()) encode(*value, dst->mutable_complex_type_data());
  if (const auto *value = src.payload.get<obj::ExprDataT>()) encode(*value, dst->mutable_expr_data());
  if (const auto *value = src.payload.get<obj::CastExprDataT>()) encode(*value, dst->mutable_cast_expr_data());
  if (const auto *value = src.payload.get<obj::LiteralDataT>()) encode(*value, dst->mutable_literal_data());
  if (const auto *value = src.payload.get<obj::CharacterLiteralDataT>()) encode(*value, dst->mutable_character_literal_data());
  if (const auto *value = src.payload.get<obj::IntegerLiteralDataT>()) encode(*value, dst->mutable_integer_literal_data());
  if (const auto *value = src.payload.get<obj::FloatingLiteralDataT>()) encode(*value, dst->mutable_floating_literal_data());
  if (const auto *value = src.payload.get<obj::StringLiteralDataT>()) encode(*value, dst->mutable_string_literal_data());
  if (const auto *value = src.payload.get<obj::CXXBoolLiteralExprDataT>()) encode(*value, dst->mutable_c_x_x_bool_literal_expr_data());
  if (const auto *value = src.payload.get<obj::CompoundLiteralExprDataT>()) encode(*value, dst->mutable_compound_literal_expr_data());
  if (const auto *value = src.payload.get<obj::InitListExprDataT>()) encode(*value, dst->mutable_init_list_expr_data());
  if (const auto *value = src.payload.get<obj::DeclRefExprDataT>()) encode(*value, dst->mutable_decl_ref_expr_data());
  if (const auto *value = src.payload.get<obj::OverloadExprDataT>()) encode(*value, dst->mutable_overload_expr_data());
  if (const auto *value = src.payload.get<obj::CXXConstructExprDataT>()) encode(*value, dst->mutable_c_x_x_construct_expr_data());
  if (const auto *value = src.payload.get<obj::CXXTemporaryObjectExprDataT>()) encode(*value, dst->mutable_c_x_x_temporary_object_expr_data());
  if (const auto *value = src.payload.get<obj::MemberExprDataT>()) encode(*value, dst->mutable_member_expr_data());
  if (const auto *value = src.payload.get<obj::MaterializeTemporaryExprDataT>()) encode(*value, dst->mutable_materialize_temporary_expr_data());
  if (const auto *value = src.payload.get<obj::BinaryOperatorDataT>()) encode(*value, dst->mutable_binary_operator_data());
  if (const auto *value = src.payload.get<obj::UnresolvedMemberExprDataT>()) encode(*value, dst->mutable_unresolved_member_expr_data());
  if (const auto *value = src.payload.get<obj::UnresolvedLookupExprDataT>()) encode(*value, dst->mutable_unresolved_lookup_expr_data());
  if (const auto *value = src.payload.get<obj::CallExprDataT>()) encode(*value, dst->mutable_call_expr_data());
  if (const auto *value = src.payload.get<obj::CXXMemberCallExprDataT>()) encode(*value, dst->mutable_c_x_x_member_call_expr_data());
  if (const auto *value = src.payload.get<obj::CXXTypeidExprDataT>()) encode(*value, dst->mutable_c_x_x_typeid_expr_data());
  if (const auto *value = src.payload.get<obj::ExplicitCastExprDataT>()) encode(*value, dst->mutable_explicit_cast_expr_data());
  if (const auto *value = src.payload.get<obj::CXXNamedCastExprDataT>()) encode(*value, dst->mutable_c_x_x_named_cast_expr_data());
  if (const auto *value = src.payload.get<obj::CXXDependentScopeMemberExprDataT>()) encode(*value, dst->mutable_c_x_x_dependent_scope_member_expr_data());
  if (const auto *value = src.payload.get<obj::UnaryOperatorDataT>()) encode(*value, dst->mutable_unary_operator_data());
  if (const auto *value = src.payload.get<obj::UnaryExprOrTypeTraitExprDataT>()) encode(*value, dst->mutable_unary_expr_or_type_trait_expr_data());
  if (const auto *value = src.payload.get<obj::CXXNewExprDataT>()) encode(*value, dst->mutable_c_x_x_new_expr_data());
  if (const auto *value = src.payload.get<obj::CXXDeleteExprDataT>()) encode(*value, dst->mutable_c_x_x_delete_expr_data());
  if (const auto *value = src.payload.get<obj::OffsetOfExprDataT>()) encode(*value, dst->mutable_offset_of_expr_data());
  if (const auto *value = src.payload.get<obj::LambdaExprDataT>()) encode(*value, dst->mutable_lambda_expr_data());
  if (const auto *value = src.payload.get<obj::PredefinedExprDataT>()) encode(*value, dst->mutable_predefined_expr_data());
  if (const auto *value = src.payload.get<obj::SizeOfPackExprDataT>()) encode(*value, dst->mutable_size_of_pack_expr_data());
  if (const auto *value = src.payload.get<obj::ArrayInitLoopExprDataT>()) encode(*value, dst->mutable_array_init_loop_expr_data());
  if (const auto *value = src.payload.get<obj::DesignatedInitExprDataT>()) encode(*value, dst->mutable_designated_init_expr_data());
  if (const auto *value = src.payload.get<obj::DependentScopeDeclRefExprDataT>()) encode(*value, dst->mutable_dependent_scope_decl_ref_expr_data());
  if (const auto *value = src.payload.get<obj::CXXNoexceptExprDataT>()) encode(*value, dst->mutable_c_x_x_noexcept_expr_data());
  if (const auto *value = src.payload.get<obj::CXXPseudoDestructorExprDataT>()) encode(*value, dst->mutable_c_x_x_pseudo_destructor_expr_data());
  if (const auto *value = src.payload.get<obj::PseudoObjectExprDataT>()) encode(*value, dst->mutable_pseudo_object_expr_data());
  if (const auto *value = src.payload.get<obj::MSPropertyRefExprDataT>()) encode(*value, dst->mutable_m_s_property_ref_expr_data());
  if (const auto *value = src.payload.get<obj::StmtDataT>()) encode(*value, dst->mutable_stmt_data());
  if (const auto *value = src.payload.get<obj::LabelStmtDataT>()) encode(*value, dst->mutable_label_stmt_data());
  if (const auto *value = src.payload.get<obj::GotoStmtDataT>()) encode(*value, dst->mutable_goto_stmt_data());
  if (const auto *value = src.payload.get<obj::AttributedStmtDataT>()) encode(*value, dst->mutable_attributed_stmt_data());
  if (const auto *value = src.payload.get<obj::AsmStmtDataT>()) encode(*value, dst->mutable_asm_stmt_data());
  if (const auto *value = src.payload.get<obj::GCCAsmStmtDataT>()) encode(*value, dst->mutable_g_c_c_asm_stmt_data());
  if (const auto *value = src.payload.get<obj::MSAsmStmtDataT>()) encode(*value, dst->mutable_m_s_asm_stmt_data());
  if (const auto *value = src.payload.get<obj::AttributeDataT>()) encode(*value, dst->mutable_attribute_data());
  if (const auto *value = src.payload.get<obj::AlignedAttrDataT>()) encode(*value, dst->mutable_aligned_attr_data());
  if (const auto *value = src.payload.get<obj::OpenCLUnrollHintAttrDataT>()) encode(*value, dst->mutable_open_c_l_unroll_hint_attr_data());
  if (const auto *value = src.payload.get<obj::FormatAttrDataT>()) encode(*value, dst->mutable_format_attr_data());
  if (const auto *value = src.payload.get<obj::NonNullAttrDataT>()) encode(*value, dst->mutable_non_null_attr_data());
  if (const auto *value = src.payload.get<obj::VisibilityAttrDataT>()) encode(*value, dst->mutable_visibility_attr_data());
}

template <typename T> inline void setRecord(const T &, pb::Record *) {
  static_assert(sizeof(T) == 0, "No protobuf record mapping for this object");
}
inline void setRecord(const obj::FileT &src, pb::Record *dst) {
  encode(src, dst->mutable_file());
}
inline void setRecord(const obj::NodeT &src, pb::Record *dst) {
  encode(src, dst->mutable_node());
}
inline void setRecord(const obj::ChildrenT &src, pb::Record *dst) {
  encode(src, dst->mutable_children());
}
inline void setRecord(const obj::NodeClassT &src, pb::Record *dst) {
  encode(src, dst->mutable_node_class());
}
inline void setRecord(const obj::TopLevelT &src, pb::Record *dst) {
  encode(src, dst->mutable_top_level());
}
inline void setRecord(const obj::IncludeT &src, pb::Record *dst) {
  encode(src, dst->mutable_include());
}
inline void setRecord(const obj::PragmaT &src, pb::Record *dst) {
  encode(src, dst->mutable_pragma());
}
inline void setRecord(const obj::TranslationUnitFileT &src, pb::Record *dst) {
  encode(src, dst->mutable_translation_unit_file());
}
inline void setRecord(const obj::CounterT &src, pb::Record *dst) {
  encode(src, dst->mutable_counter());
}
inline void setRecord(const obj::LanguageT &src, pb::Record *dst) {
  encode(src, dst->mutable_language());
}

} // namespace clava::proto
