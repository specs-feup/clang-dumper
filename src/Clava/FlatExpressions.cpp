#include "FlatSupport.h"

#include "clang/AST/ExprCXX.h"
#include "clang/AST/StmtCXX.h"
#include "llvm/ADT/STLForwardCompat.h"

#include <stdexcept>

namespace clava::flat {
namespace {

fb::ValueKind valueKind(clang::ExprValueKind kind) {
  switch (kind) {
  case clang::VK_PRValue:
    return fb::ValueKind::R_VALUE;
  case clang::VK_LValue:
    return fb::ValueKind::L_VALUE;
  case clang::VK_XValue:
    return fb::ValueKind::X_VALUE;
  }
  throw std::invalid_argument("Unsupported Clang expression value kind");
}

fb::ObjectKind objectKind(clang::ExprObjectKind kind) {
  switch (kind) {
  case clang::OK_Ordinary:
    return fb::ObjectKind::ORDINARY;
  case clang::OK_BitField:
    return fb::ObjectKind::BIT_FIELD;
  case clang::OK_ObjCProperty:
    return fb::ObjectKind::OBJ_C_PROPERTY;
  case clang::OK_ObjCSubscript:
    return fb::ObjectKind::OBJ_C_SUBSCRIPT;
  case clang::OK_VectorComponent:
    return fb::ObjectKind::VECTOR_COMPONENT;
  }
  throw std::invalid_argument("Unsupported Clang expression object kind");
}

template <typename Node>
void templateArguments(std::vector<std::unique_ptr<fb::TemplateArgumentT>> &out,
                       const Node *node, Context &c) {
  if (!node->hasExplicitTemplateArgs()) {
    return;
  }
  const auto *args = node->getTemplateArgs();
  for (unsigned i = 0; i < node->getNumTemplateArgs(); ++i) {
    out.push_back(makeTemplateArgument((args + i)->getArgument(), c));
  }
}

} // namespace

std::unique_ptr<fb::ExprDataT> makeExprData(const clang::Expr *node,
                                            Context &c) {
  auto out = std::make_unique<fb::ExprDataT>();
  out->base = makeStmtData(node, c);
  out->type = wireId(clava::getId(node->getType(), c.id));
  out->value_kind = valueKind(node->getValueKind());
  out->object_kind = objectKind(node->getObjectKind());
  out->is_default_argument = node->isDefaultArgument();
  return out;
}

std::unique_ptr<fb::CastExprDataT> makeCastExprData(const clang::CastExpr *node,
                                                   Context &c) {
  auto out = std::make_unique<fb::CastExprDataT>();
  out->base = makeExprData(node, c);
  out->cast_kind = enumValue<fb::CastKind>(clava::CAST_KIND[node->getCastKind()]);
  return out;
}

std::unique_ptr<fb::LiteralDataT> makeLiteralData(const clang::Expr *node,
                                                  Context &c) {
  auto out = std::make_unique<fb::LiteralDataT>();
  out->base = makeExprData(node, c);
  out->source_literal = sourceText(node->getSourceRange(), c);
  return out;
}

std::unique_ptr<fb::CharacterLiteralDataT> makeCharacterLiteralData(
    const clang::CharacterLiteral *node, Context &c) {
  auto out = std::make_unique<fb::CharacterLiteralDataT>();
  out->base = makeLiteralData(node, c);
  out->value = node->getValue();
  out->kind = enumValue<fb::CharacterKind>(
      clava::CHARACTER_LITERAL_KIND[llvm::to_underlying(node->getKind())]);
  return out;
}

std::unique_ptr<fb::IntegerLiteralDataT> makeIntegerLiteralData(
    const clang::IntegerLiteral *node, Context &c) {
  auto out = std::make_unique<fb::IntegerLiteralDataT>();
  out->base = makeLiteralData(node, c);
  const bool isSigned = node->getType()->isSignedIntegerType();
  llvm::SmallString<0> value;
  node->getValue().toString(value, 10, isSigned);
  out->value = value.str().str();
  return out;
}

std::unique_ptr<fb::FloatingLiteralDataT> makeFloatingLiteralData(
    const clang::FloatingLiteral *node, Context &c) {
  auto out = std::make_unique<fb::FloatingLiteralDataT>();
  out->base = makeLiteralData(node, c);
  // Match raw_ostream's decimal precision in the existing text protocol.
  // The source spelling remains separately preserved in LiteralData.
  std::string text;
  llvm::raw_string_ostream stream(text);
  stream << node->getValueAsApproximateDouble();
  out->value = std::stod(text);
  return out;
}

std::unique_ptr<fb::StringLiteralDataT> makeStringLiteralData(
    const clang::StringLiteral *node, Context &c) {
  auto out = std::make_unique<fb::StringLiteralDataT>();
  out->base = makeLiteralData(node, c);
  out->string_kind = enumValue<fb::StringKind>(
      clava::STRING_KIND[llvm::to_underlying(node->getKind())]);
  out->length = node->getLength();
  out->char_byte_width = node->getCharByteWidth();
  const auto bytes = node->getBytes();
  out->string_bytes.assign(bytes.bytes_begin(), bytes.bytes_end());
  return out;
}

std::unique_ptr<fb::CXXBoolLiteralExprDataT> makeCXXBoolLiteralExprData(
    const clang::CXXBoolLiteralExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXBoolLiteralExprDataT>();
  out->base = makeLiteralData(node, c);
  out->value = node->getValue();
  return out;
}

std::unique_ptr<fb::CompoundLiteralExprDataT> makeCompoundLiteralExprData(
    const clang::CompoundLiteralExpr *node, Context &c) {
  auto out = std::make_unique<fb::CompoundLiteralExprDataT>();
  out->base = makeLiteralData(node, c);
  out->is_file_scope = node->isFileScope();
  return out;
}

std::unique_ptr<fb::InitListExprDataT> makeInitListExprData(
    const clang::InitListExpr *node, Context &c) {
  auto out = std::make_unique<fb::InitListExprDataT>();
  out->base = makeExprData(node, c);
  out->array_filler = wireId(clava::getId(node->getArrayFiller(), c.id));
  out->is_explicit = const_cast<clang::InitListExpr *>(node)->isExplicit();
  out->is_string_literal_init = node->isStringLiteralInit();
  out->syntactic_form = wireId(clava::getId(node->getSyntacticForm(), c.id));
  out->semantic_form = wireId(clava::getId(node->getSemanticForm(), c.id));
  return out;
}

std::unique_ptr<fb::DeclRefExprDataT> makeDeclRefExprData(
    const clang::DeclRefExpr *node, Context &c) {
  auto out = std::make_unique<fb::DeclRefExprDataT>();
  out->base = makeExprData(node, c);
  out->qualifier = qualifierString(node->getQualifier(), c);
  templateArguments(out->template_arguments, node, c);
  out->decl = wireId(clava::getId(node->getDecl(), c.id));
  return out;
}

std::unique_ptr<fb::OverloadExprDataT> makeOverloadExprData(
    const clang::OverloadExpr *node, Context &c) {
  auto out = std::make_unique<fb::OverloadExprDataT>();
  out->base = makeExprData(node, c);
  out->qualifier = qualifierString(node->getQualifier(), c);
  out->name = node->getName().getAsString();
  for (auto it = node->decls_begin(); it != node->decls_end(); ++it) {
    out->unresolved_decls.push_back(wireId(clava::getId(*it, c.id)));
  }
  templateArguments(out->template_arguments, node, c);
  return out;
}

std::unique_ptr<fb::CXXConstructExprDataT> makeCXXConstructExprData(
    const clang::CXXConstructExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXConstructExprDataT>();
  out->base = makeExprData(node, c);
  out->is_elidable = node->isElidable();
  out->requires_zero_initialization = node->requiresZeroInitialization();
  out->is_list_initialization = node->isListInitialization();
  out->is_std_list_initialization = node->isStdInitListInitialization();
  out->construction_kind = enumValue<fb::ConstructionKind>(
      clava::CONSTRUCTION_KIND[llvm::to_underlying(node->getConstructionKind())]);
  out->is_temporary_object =
      node->isTemporaryObject(*c.ast, node->getConstructor()->getParent());
  out->constructor_decl = wireId(clava::getId(node->getConstructor(), c.id));
  return out;
}

std::unique_ptr<fb::CXXTemporaryObjectExprDataT> makeCXXTemporaryObjectExprData(
    const clang::CXXTemporaryObjectExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXTemporaryObjectExprDataT>();
  out->base = makeCXXConstructExprData(node, c);
  return out;
}

std::unique_ptr<fb::MemberExprDataT> makeMemberExprData(
    const clang::MemberExpr *node, Context &c) {
  auto out = std::make_unique<fb::MemberExprDataT>();
  out->base = makeExprData(node, c);
  out->is_arrow = node->isArrow();
  out->member_name = node->getMemberNameInfo().getAsString();
  out->member_decl = wireId(clava::getId(node->getMemberDecl(), c.id));
  out->found_decl = wireId(clava::getId(node->getFoundDecl().getDecl(), c.id));
  out->found_decl_access_specifier = enumValue<fb::AccessSpecifier>(
      clava::ACCESS_SPECIFIER[node->getFoundDecl().getAccess()]);
  return out;
}

std::unique_ptr<fb::MaterializeTemporaryExprDataT>
makeMaterializeTemporaryExprData(const clang::MaterializeTemporaryExpr *node,
                                 Context &c) {
  auto out = std::make_unique<fb::MaterializeTemporaryExprDataT>();
  out->base = makeExprData(node, c);
  out->extending_decl = wireId(clava::getId(node->getExtendingDecl(), c.id));
  return out;
}

std::unique_ptr<fb::BinaryOperatorDataT> makeBinaryOperatorData(
    const clang::BinaryOperator *node, Context &c) {
  auto out = std::make_unique<fb::BinaryOperatorDataT>();
  out->base = makeExprData(node, c);
  out->op = enumValue<fb::BinaryOperatorKind>(
      clava::BINARY_OPERATOR_KIND[node->getOpcode()]);
  return out;
}

std::unique_ptr<fb::UnresolvedMemberExprDataT> makeUnresolvedMemberExprData(
    const clang::UnresolvedMemberExpr *node, Context &c) {
  auto out = std::make_unique<fb::UnresolvedMemberExprDataT>();
  out->base = makeOverloadExprData(node, c);
  return out;
}

std::unique_ptr<fb::UnresolvedLookupExprDataT> makeUnresolvedLookupExprData(
    const clang::UnresolvedLookupExpr *node, Context &c) {
  auto out = std::make_unique<fb::UnresolvedLookupExprDataT>();
  out->base = makeOverloadExprData(node, c);
  out->requires_adl = node->requiresADL();
  return out;
}

std::unique_ptr<fb::CallExprDataT> makeCallExprData(
    const clang::CallExpr *node, Context &c) {
  auto out = std::make_unique<fb::CallExprDataT>();
  out->base = makeExprData(node, c);
  out->direct_callee = wireId(clava::getId(node->getDirectCallee(), c.id));
  return out;
}

std::unique_ptr<fb::CXXMemberCallExprDataT> makeCXXMemberCallExprData(
    const clang::CXXMemberCallExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXMemberCallExprDataT>();
  out->base = makeCallExprData(node, c);
  out->method_decl = wireId(clava::getId(node->getMethodDecl(), c.id));
  return out;
}

std::unique_ptr<fb::CXXTypeidExprDataT> makeCXXTypeidExprData(
    const clang::CXXTypeidExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXTypeidExprDataT>();
  out->base = makeExprData(node, c);
  out->is_type_operand = node->isTypeOperand();
  if (node->isTypeOperand()) {
    out->operand = wireId(clava::getId(node->getTypeOperand(*c.ast), c.id));
  } else {
    out->operand = wireId(clava::getId(node->getExprOperand(), c.id));
  }
  return out;
}

std::unique_ptr<fb::ExplicitCastExprDataT> makeExplicitCastExprData(
    const clang::ExplicitCastExpr *node, Context &c) {
  auto out = std::make_unique<fb::ExplicitCastExprDataT>();
  out->base = makeCastExprData(node, c);
  out->type_as_written =
      wireId(clava::getId(node->getTypeAsWritten(), c.id));
  return out;
}

std::unique_ptr<fb::CXXNamedCastExprDataT> makeCXXNamedCastExprData(
    const clang::CXXNamedCastExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXNamedCastExprDataT>();
  out->base = makeExplicitCastExprData(node, c);
  out->cast_name = node->getCastName();
  return out;
}

std::unique_ptr<fb::CXXDependentScopeMemberExprDataT>
makeCXXDependentScopeMemberExprData(
    const clang::CXXDependentScopeMemberExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXDependentScopeMemberExprDataT>();
  out->base = makeExprData(node, c);
  out->is_arrow = node->isArrow();
  out->member_name = node->getMemberNameInfo().getAsString();
  out->is_implicit_access = node->isImplicitAccess();
  out->qualifier = qualifierString(node->getQualifier(), c);
  out->has_template_keyword = node->hasTemplateKeyword();
  templateArguments(out->template_arguments, node, c);
  return out;
}

std::unique_ptr<fb::UnaryOperatorDataT> makeUnaryOperatorData(
    const clang::UnaryOperator *node, Context &c) {
  auto out = std::make_unique<fb::UnaryOperatorDataT>();
  out->base = makeExprData(node, c);
  out->op = enumValue<fb::UnaryOperatorKind>(
      clava::UNARY_OPERATOR_KIND[node->getOpcode()]);
  out->position = enumValue<fb::UnaryOperatorPosition>(
      node->isPostfix() ? "POSTFIX" : "PREFIX");
  return out;
}

std::unique_ptr<fb::UnaryExprOrTypeTraitExprDataT>
makeUnaryExprOrTypeTraitExprData(const clang::UnaryExprOrTypeTraitExpr *node,
                                 Context &c) {
  auto out = std::make_unique<fb::UnaryExprOrTypeTraitExprDataT>();
  out->base = makeExprData(node, c);
  out->kind = enumValue<fb::UnaryExprOrTypeTrait>(
      clava::UETT_KIND[node->getKind()]);
  out->is_argument_type = node->isArgumentType();
  out->arg_type = node->isArgumentType()?wireId(clava::getId(node->getArgumentType(), c.id)):-1;
  out->source_literal = sourceText(node->getSourceRange(), c);
  return out;
}

std::unique_ptr<fb::CXXNewExprDataT> makeCXXNewExprData(
    const clang::CXXNewExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXNewExprDataT>();
  out->base = makeExprData(node, c);
  out->is_global = node->isGlobalNew();
  out->is_array = node->isArray();
  out->initialization_present = node->hasInitializer();
  out->init_style = enumValue<fb::NewInitStyle>(
      clava::NEW_INIT_STYLE[llvm::to_underlying(node->getInitializationStyle())]);
  out->initializer = wireId(clava::getId(node->getInitializer(), c.id));
  out->construct_expr = wireId(clava::getId(node->getConstructExpr(), c.id));
  out->array_size = wireId(clava::getId(node->getArraySize(), c.id));
  out->operator_new = wireId(clava::getId(node->getOperatorNew(), c.id));
  return out;
}

std::unique_ptr<fb::CXXDeleteExprDataT> makeCXXDeleteExprData(
    const clang::CXXDeleteExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXDeleteExprDataT>();
  out->base = makeExprData(node, c);
  out->is_global = node->isGlobalDelete();
  out->is_array = node->isArrayForm();
  out->is_array_as_written = node->isArrayFormAsWritten();
  return out;
}

std::unique_ptr<fb::OffsetOfExprDataT> makeOffsetOfExprData(
    const clang::OffsetOfExpr *node, Context &c) {
  auto out = std::make_unique<fb::OffsetOfExprDataT>();
  out->base = makeExprData(node, c);
  out->source_type =
      wireId(clava::getId(node->getTypeSourceInfo()->getType(), c.id));
  for (unsigned i = 0; i < node->getNumComponents(); ++i) {
    out->components.push_back(makeOffsetOfComponent(node, i, c));
  }
  return out;
}

std::unique_ptr<fb::LambdaExprDataT> makeLambdaExprData(
    const clang::LambdaExpr *node, Context &c) {
  auto out = std::make_unique<fb::LambdaExprDataT>();
  out->base = makeExprData(node, c);
  out->is_generic_lambda = node->isGenericLambda();
  out->is_mutable = node->isMutable();
  out->has_explicit_parameters = node->hasExplicitParameters();
  out->has_explicit_result_type = node->hasExplicitResultType();
  out->capture_default = enumValue<fb::LambdaCaptureDefault>(
      clava::LAMBDA_CAPTURE_DEFAULT[node->getCaptureDefault()]);
  out->lambda_class = wireId(clava::getId(node->getLambdaClass(), c.id));
  for (const auto capture : node->captures()) {
    out->capture_kinds.push_back(enumValue<fb::LambdaCaptureKind>(
        clava::LAMBDA_CAPTURE_KIND[capture.getCaptureKind()]));
  }
  return out;
}

std::unique_ptr<fb::PredefinedExprDataT> makePredefinedExprData(
    const clang::PredefinedExpr *node, Context &c) {
  auto out = std::make_unique<fb::PredefinedExprDataT>();
  out->base = makeExprData(node, c);
  out->predefined_type = enumValue<fb::PredefinedIdType>(
      clava::PREDEFINED_ID_TYPE[llvm::to_underlying(node->getIdentKind())]);
  return out;
}

std::unique_ptr<fb::SizeOfPackExprDataT> makeSizeOfPackExprData(
    const clang::SizeOfPackExpr *node, Context &c) {
  auto out = std::make_unique<fb::SizeOfPackExprDataT>();
  out->base = makeExprData(node, c);
  out->is_partially_substituted = node->isPartiallySubstituted();
  out->pack = wireId(clava::getId(node->getPack(), c.id));
  if (node->isPartiallySubstituted()) {
    for (const auto &arg : node->getPartialArguments()) {
      out->partial_arguments.push_back(makeTemplateArgument(arg, c));
    }
  }
  return out;
}

std::unique_ptr<fb::ArrayInitLoopExprDataT> makeArrayInitLoopExprData(
    const clang::ArrayInitLoopExpr *node, Context &c) {
  auto out = std::make_unique<fb::ArrayInitLoopExprDataT>();
  out->base = makeExprData(node, c);
  return out;
}

std::unique_ptr<fb::DesignatedInitExprDataT> makeDesignatedInitExprData(
    const clang::DesignatedInitExpr *node, Context &c) {
  auto out = std::make_unique<fb::DesignatedInitExprDataT>();
  out->base = makeExprData(node, c);
  out->uses_gnu_syntax = node->usesGNUSyntax();
  for (unsigned i = 0; i < node->size(); ++i) {
    auto *designator = node->getDesignator(i);
    if (!designator->isFieldDesignator() || designator->getFieldName() != nullptr) {
      out->designators.push_back(makeDesignator(designator, c));
    }
  }
  return out;
}

std::unique_ptr<fb::DependentScopeDeclRefExprDataT>
makeDependentScopeDeclRefExprData(
    const clang::DependentScopeDeclRefExpr *node, Context &c) {
  auto out = std::make_unique<fb::DependentScopeDeclRefExprDataT>();
  out->base = makeExprData(node, c);
  out->decl_name = node->getDeclName().getAsString();
  out->qualifier = qualifierString(node->getQualifier(), c);
  out->has_template_keyword = node->hasTemplateKeyword();
  templateArguments(out->template_arguments, node, c);
  return out;
}

std::unique_ptr<fb::CXXNoexceptExprDataT> makeCXXNoexceptExprData(
    const clang::CXXNoexceptExpr *node, Context &c) {
  auto out = std::make_unique<fb::CXXNoexceptExprDataT>();
  out->base = makeExprData(node, c);
  out->value = node->getValue();
  return out;
}

std::unique_ptr<fb::CXXPseudoDestructorExprDataT>
makeCXXPseudoDestructorExprData(const clang::CXXPseudoDestructorExpr *node,
                                Context &c) {
  auto out = std::make_unique<fb::CXXPseudoDestructorExprDataT>();
  out->base = makeExprData(node, c);
  out->qualifier = node->hasQualifier() ? qualifierString(node->getQualifier(), c)
                                        : std::string();
  out->is_arrow = node->isArrow();
  out->destroyed_type = wireId(clava::getId(node->getDestroyedType(), c.id));
  return out;
}

std::unique_ptr<fb::PseudoObjectExprDataT> makePseudoObjectExprData(
    const clang::PseudoObjectExpr *node, Context &c) {
  auto out = std::make_unique<fb::PseudoObjectExprDataT>();
  out->base = makeExprData(node, c);
  auto index = node->getResultExprIndex();
  out->result_expr_index = index == clang::PseudoObjectExpr::NoResult
      ? -1 : static_cast<int>(index);
  return out;
}

std::unique_ptr<fb::MSPropertyRefExprDataT> makeMSPropertyRefExprData(
    const clang::MSPropertyRefExpr *node, Context &c) {
  auto out = std::make_unique<fb::MSPropertyRefExprDataT>();
  out->base = makeExprData(node, c);
  out->base_expr = wireId(clava::getId(node->getBaseExpr(), c.id));
  out->property_decl = wireId(clava::getId(node->getPropertyDecl(), c.id));
  out->is_implicit_access = node->isImplicitAccess();
  out->is_arrow = node->isArrow();
  return out;
}

// The legacy dumper maps concrete cast/operator classes to a parent section.
// Keep direct builders for those classes so the dispatch layer can preserve the
// same aliases without manufacturing a second wire payload.
std::unique_ptr<fb::CastExprDataT> makeCXXFunctionalCastExprData(
    const clang::CXXFunctionalCastExpr *node, Context &c) {
  return makeCastExprData(node, c);
}

std::unique_ptr<fb::ExplicitCastExprDataT> makeCStyleCastExprData(
    const clang::CStyleCastExpr *node, Context &c) {
  return makeExplicitCastExprData(node, c);
}

std::unique_ptr<fb::CXXNamedCastExprDataT> makeCXXAddrspaceCastExprData(
    const clang::CXXAddrspaceCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<fb::CXXNamedCastExprDataT> makeCXXConstCastExprData(
    const clang::CXXConstCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<fb::CXXNamedCastExprDataT> makeCXXDynamicCastExprData(
    const clang::CXXDynamicCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<fb::CXXNamedCastExprDataT> makeCXXReinterpretCastExprData(
    const clang::CXXReinterpretCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<fb::CXXNamedCastExprDataT> makeCXXStaticCastExprData(
    const clang::CXXStaticCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<fb::CallExprDataT> makeCXXOperatorCallExprData(
    const clang::CXXOperatorCallExpr *node, Context &c) {
  return makeCallExprData(node, c);
}

std::unique_ptr<fb::CallExprDataT> makeUserDefinedLiteralData(
    const clang::UserDefinedLiteral *node, Context &c) {
  return makeCallExprData(node, c);
}

} // namespace clava::flat
