#include "ProtoSupport.h"

#include "clang/AST/ExprCXX.h"
#include "clang/AST/StmtCXX.h"
#include "llvm/ADT/STLForwardCompat.h"

#include <stdexcept>

namespace clava::proto {
namespace {

obj::ValueKind valueKind(clang::ExprValueKind kind) {
  switch (kind) {
  case clang::VK_PRValue:
    return obj::ValueKind::R_VALUE;
  case clang::VK_LValue:
    return obj::ValueKind::L_VALUE;
  case clang::VK_XValue:
    return obj::ValueKind::X_VALUE;
  }
  throw std::invalid_argument("Unsupported Clang expression value kind");
}

obj::ObjectKind objectKind(clang::ExprObjectKind kind) {
  switch (kind) {
  case clang::OK_Ordinary:
    return obj::ObjectKind::ORDINARY;
  case clang::OK_BitField:
    return obj::ObjectKind::BIT_FIELD;
  case clang::OK_ObjCProperty:
    return obj::ObjectKind::OBJ_C_PROPERTY;
  case clang::OK_ObjCSubscript:
    return obj::ObjectKind::OBJ_C_SUBSCRIPT;
  case clang::OK_VectorComponent:
    return obj::ObjectKind::VECTOR_COMPONENT;
  }
  throw std::invalid_argument("Unsupported Clang expression object kind");
}

template <typename Node>
void templateArguments(std::vector<std::unique_ptr<obj::TemplateArgumentT>> &out,
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

std::unique_ptr<obj::ExprDataT> makeExprData(const clang::Expr *node,
                                            Context &c) {
  auto out = std::make_unique<obj::ExprDataT>();
  out->base = makeStmtData(node, c);
  out->type = wireId(clava::getId(node->getType(), c.id));
  out->value_kind = valueKind(node->getValueKind());
  out->object_kind = objectKind(node->getObjectKind());
  out->is_default_argument = node->isDefaultArgument();
  return out;
}

std::unique_ptr<obj::CastExprDataT> makeCastExprData(const clang::CastExpr *node,
                                                   Context &c) {
  auto out = std::make_unique<obj::CastExprDataT>();
  out->base = makeExprData(node, c);
  out->cast_kind = enumValue<obj::CastKind>(clava::CAST_KIND[node->getCastKind()]);
  return out;
}

std::unique_ptr<obj::LiteralDataT> makeLiteralData(const clang::Expr *node,
                                                  Context &c) {
  auto out = std::make_unique<obj::LiteralDataT>();
  out->base = makeExprData(node, c);
  out->source_literal = sourceText(node->getSourceRange(), c);
  return out;
}

std::unique_ptr<obj::CharacterLiteralDataT> makeCharacterLiteralData(
    const clang::CharacterLiteral *node, Context &c) {
  auto out = std::make_unique<obj::CharacterLiteralDataT>();
  out->base = makeLiteralData(node, c);
  out->value = node->getValue();
  out->kind = enumValue<obj::CharacterKind>(
      clava::CHARACTER_LITERAL_KIND[llvm::to_underlying(node->getKind())]);
  return out;
}

std::unique_ptr<obj::IntegerLiteralDataT> makeIntegerLiteralData(
    const clang::IntegerLiteral *node, Context &c) {
  auto out = std::make_unique<obj::IntegerLiteralDataT>();
  out->base = makeLiteralData(node, c);
  const bool isSigned = node->getType()->isSignedIntegerType();
  llvm::SmallString<0> value;
  node->getValue().toString(value, 10, isSigned);
  out->value = value.str().str();
  return out;
}

std::unique_ptr<obj::FloatingLiteralDataT> makeFloatingLiteralData(
    const clang::FloatingLiteral *node, Context &c) {
  auto out = std::make_unique<obj::FloatingLiteralDataT>();
  out->base = makeLiteralData(node, c);
  // Match raw_ostream's decimal precision in the existing text protocol.
  // The source spelling remains separately preserved in LiteralData.
  std::string text;
  llvm::raw_string_ostream stream(text);
  stream << node->getValueAsApproximateDouble();
  out->value = std::stod(text);
  return out;
}

std::unique_ptr<obj::StringLiteralDataT> makeStringLiteralData(
    const clang::StringLiteral *node, Context &c) {
  auto out = std::make_unique<obj::StringLiteralDataT>();
  out->base = makeLiteralData(node, c);
  out->string_kind = enumValue<obj::StringKind>(
      clava::STRING_KIND[llvm::to_underlying(node->getKind())]);
  out->length = node->getLength();
  out->char_byte_width = node->getCharByteWidth();
  const auto bytes = node->getBytes();
  out->string_bytes.assign(bytes.bytes_begin(), bytes.bytes_end());
  return out;
}

std::unique_ptr<obj::CXXBoolLiteralExprDataT> makeCXXBoolLiteralExprData(
    const clang::CXXBoolLiteralExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXBoolLiteralExprDataT>();
  out->base = makeLiteralData(node, c);
  out->value = node->getValue();
  return out;
}

std::unique_ptr<obj::CompoundLiteralExprDataT> makeCompoundLiteralExprData(
    const clang::CompoundLiteralExpr *node, Context &c) {
  auto out = std::make_unique<obj::CompoundLiteralExprDataT>();
  out->base = makeLiteralData(node, c);
  out->is_file_scope = node->isFileScope();
  return out;
}

std::unique_ptr<obj::InitListExprDataT> makeInitListExprData(
    const clang::InitListExpr *node, Context &c) {
  auto out = std::make_unique<obj::InitListExprDataT>();
  out->base = makeExprData(node, c);
  out->array_filler = wireId(clava::getId(node->getArrayFiller(), c.id));
  out->is_explicit = const_cast<clang::InitListExpr *>(node)->isExplicit();
  out->is_string_literal_init = node->isStringLiteralInit();
  out->syntactic_form = wireId(clava::getId(node->getSyntacticForm(), c.id));
  out->semantic_form = wireId(clava::getId(node->getSemanticForm(), c.id));
  return out;
}

std::unique_ptr<obj::DeclRefExprDataT> makeDeclRefExprData(
    const clang::DeclRefExpr *node, Context &c) {
  auto out = std::make_unique<obj::DeclRefExprDataT>();
  out->base = makeExprData(node, c);
  out->qualifier = qualifierString(node->getQualifier(), c);
  templateArguments(out->template_arguments, node, c);
  out->decl = wireId(clava::getId(node->getDecl(), c.id));
  return out;
}

std::unique_ptr<obj::OverloadExprDataT> makeOverloadExprData(
    const clang::OverloadExpr *node, Context &c) {
  auto out = std::make_unique<obj::OverloadExprDataT>();
  out->base = makeExprData(node, c);
  out->qualifier = qualifierString(node->getQualifier(), c);
  out->name = node->getName().getAsString();
  for (auto it = node->decls_begin(); it != node->decls_end(); ++it) {
    out->unresolved_decls.push_back(wireId(clava::getId(*it, c.id)));
  }
  templateArguments(out->template_arguments, node, c);
  return out;
}

std::unique_ptr<obj::CXXConstructExprDataT> makeCXXConstructExprData(
    const clang::CXXConstructExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXConstructExprDataT>();
  out->base = makeExprData(node, c);
  out->is_elidable = node->isElidable();
  out->requires_zero_initialization = node->requiresZeroInitialization();
  out->is_list_initialization = node->isListInitialization();
  out->is_std_list_initialization = node->isStdInitListInitialization();
  out->construction_kind = enumValue<obj::ConstructionKind>(
      clava::CONSTRUCTION_KIND[llvm::to_underlying(node->getConstructionKind())]);
  out->is_temporary_object =
      node->isTemporaryObject(*c.ast, node->getConstructor()->getParent());
  out->constructor_decl = wireId(clava::getId(node->getConstructor(), c.id));
  return out;
}

std::unique_ptr<obj::CXXTemporaryObjectExprDataT> makeCXXTemporaryObjectExprData(
    const clang::CXXTemporaryObjectExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXTemporaryObjectExprDataT>();
  out->base = makeCXXConstructExprData(node, c);
  return out;
}

std::unique_ptr<obj::MemberExprDataT> makeMemberExprData(
    const clang::MemberExpr *node, Context &c) {
  auto out = std::make_unique<obj::MemberExprDataT>();
  out->base = makeExprData(node, c);
  out->is_arrow = node->isArrow();
  out->member_name = node->getMemberNameInfo().getAsString();
  out->member_decl = wireId(clava::getId(node->getMemberDecl(), c.id));
  out->found_decl = wireId(clava::getId(node->getFoundDecl().getDecl(), c.id));
  out->found_decl_access_specifier = enumValue<obj::AccessSpecifier>(
      clava::ACCESS_SPECIFIER[node->getFoundDecl().getAccess()]);
  return out;
}

std::unique_ptr<obj::MaterializeTemporaryExprDataT>
makeMaterializeTemporaryExprData(const clang::MaterializeTemporaryExpr *node,
                                 Context &c) {
  auto out = std::make_unique<obj::MaterializeTemporaryExprDataT>();
  out->base = makeExprData(node, c);
  out->extending_decl = wireId(clava::getId(node->getExtendingDecl(), c.id));
  return out;
}

std::unique_ptr<obj::BinaryOperatorDataT> makeBinaryOperatorData(
    const clang::BinaryOperator *node, Context &c) {
  auto out = std::make_unique<obj::BinaryOperatorDataT>();
  out->base = makeExprData(node, c);
  out->op = enumValue<obj::BinaryOperatorKind>(
      clava::BINARY_OPERATOR_KIND[node->getOpcode()]);
  return out;
}

std::unique_ptr<obj::UnresolvedMemberExprDataT> makeUnresolvedMemberExprData(
    const clang::UnresolvedMemberExpr *node, Context &c) {
  auto out = std::make_unique<obj::UnresolvedMemberExprDataT>();
  out->base = makeOverloadExprData(node, c);
  return out;
}

std::unique_ptr<obj::UnresolvedLookupExprDataT> makeUnresolvedLookupExprData(
    const clang::UnresolvedLookupExpr *node, Context &c) {
  auto out = std::make_unique<obj::UnresolvedLookupExprDataT>();
  out->base = makeOverloadExprData(node, c);
  out->requires_adl = node->requiresADL();
  return out;
}

std::unique_ptr<obj::CallExprDataT> makeCallExprData(
    const clang::CallExpr *node, Context &c) {
  auto out = std::make_unique<obj::CallExprDataT>();
  out->base = makeExprData(node, c);
  out->direct_callee = wireId(clava::getId(node->getDirectCallee(), c.id));
  return out;
}

std::unique_ptr<obj::CXXMemberCallExprDataT> makeCXXMemberCallExprData(
    const clang::CXXMemberCallExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXMemberCallExprDataT>();
  out->base = makeCallExprData(node, c);
  out->method_decl = wireId(clava::getId(node->getMethodDecl(), c.id));
  return out;
}

std::unique_ptr<obj::CXXTypeidExprDataT> makeCXXTypeidExprData(
    const clang::CXXTypeidExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXTypeidExprDataT>();
  out->base = makeExprData(node, c);
  out->is_type_operand = node->isTypeOperand();
  if (node->isTypeOperand()) {
    out->operand = wireId(clava::getId(node->getTypeOperand(*c.ast), c.id));
  } else {
    out->operand = wireId(clava::getId(node->getExprOperand(), c.id));
  }
  return out;
}

std::unique_ptr<obj::ExplicitCastExprDataT> makeExplicitCastExprData(
    const clang::ExplicitCastExpr *node, Context &c) {
  auto out = std::make_unique<obj::ExplicitCastExprDataT>();
  out->base = makeCastExprData(node, c);
  out->type_as_written =
      wireId(clava::getId(node->getTypeAsWritten(), c.id));
  return out;
}

std::unique_ptr<obj::CXXNamedCastExprDataT> makeCXXNamedCastExprData(
    const clang::CXXNamedCastExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXNamedCastExprDataT>();
  out->base = makeExplicitCastExprData(node, c);
  out->cast_name = node->getCastName();
  return out;
}

std::unique_ptr<obj::CXXDependentScopeMemberExprDataT>
makeCXXDependentScopeMemberExprData(
    const clang::CXXDependentScopeMemberExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXDependentScopeMemberExprDataT>();
  out->base = makeExprData(node, c);
  out->is_arrow = node->isArrow();
  out->member_name = node->getMemberNameInfo().getAsString();
  out->is_implicit_access = node->isImplicitAccess();
  out->qualifier = qualifierString(node->getQualifier(), c);
  out->has_template_keyword = node->hasTemplateKeyword();
  templateArguments(out->template_arguments, node, c);
  return out;
}

std::unique_ptr<obj::UnaryOperatorDataT> makeUnaryOperatorData(
    const clang::UnaryOperator *node, Context &c) {
  auto out = std::make_unique<obj::UnaryOperatorDataT>();
  out->base = makeExprData(node, c);
  out->op = enumValue<obj::UnaryOperatorKind>(
      clava::UNARY_OPERATOR_KIND[node->getOpcode()]);
  out->position = enumValue<obj::UnaryOperatorPosition>(
      node->isPostfix() ? "POSTFIX" : "PREFIX");
  return out;
}

std::unique_ptr<obj::UnaryExprOrTypeTraitExprDataT>
makeUnaryExprOrTypeTraitExprData(const clang::UnaryExprOrTypeTraitExpr *node,
                                 Context &c) {
  auto out = std::make_unique<obj::UnaryExprOrTypeTraitExprDataT>();
  out->base = makeExprData(node, c);
  out->kind = enumValue<obj::UnaryExprOrTypeTrait>(
      clava::UETT_KIND[node->getKind()]);
  out->is_argument_type = node->isArgumentType();
  out->arg_type = node->isArgumentType()?wireId(clava::getId(node->getArgumentType(), c.id)):-1;
  out->source_literal = sourceText(node->getSourceRange(), c);
  return out;
}

std::unique_ptr<obj::CXXNewExprDataT> makeCXXNewExprData(
    const clang::CXXNewExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXNewExprDataT>();
  out->base = makeExprData(node, c);
  out->is_global = node->isGlobalNew();
  out->is_array = node->isArray();
  out->initialization_present = node->hasInitializer();
  out->init_style = enumValue<obj::NewInitStyle>(
      clava::NEW_INIT_STYLE[llvm::to_underlying(node->getInitializationStyle())]);
  out->initializer = wireId(clava::getId(node->getInitializer(), c.id));
  out->construct_expr = wireId(clava::getId(node->getConstructExpr(), c.id));
  out->array_size = wireId(clava::getId(node->getArraySize(), c.id));
  out->operator_new = wireId(clava::getId(node->getOperatorNew(), c.id));
  return out;
}

std::unique_ptr<obj::CXXDeleteExprDataT> makeCXXDeleteExprData(
    const clang::CXXDeleteExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXDeleteExprDataT>();
  out->base = makeExprData(node, c);
  out->is_global = node->isGlobalDelete();
  out->is_array = node->isArrayForm();
  out->is_array_as_written = node->isArrayFormAsWritten();
  return out;
}

std::unique_ptr<obj::OffsetOfExprDataT> makeOffsetOfExprData(
    const clang::OffsetOfExpr *node, Context &c) {
  auto out = std::make_unique<obj::OffsetOfExprDataT>();
  out->base = makeExprData(node, c);
  out->source_type =
      wireId(clava::getId(node->getTypeSourceInfo()->getType(), c.id));
  for (unsigned i = 0; i < node->getNumComponents(); ++i) {
    out->components.push_back(makeOffsetOfComponent(node, i, c));
  }
  return out;
}

std::unique_ptr<obj::LambdaExprDataT> makeLambdaExprData(
    const clang::LambdaExpr *node, Context &c) {
  auto out = std::make_unique<obj::LambdaExprDataT>();
  out->base = makeExprData(node, c);
  out->is_generic_lambda = node->isGenericLambda();
  out->is_mutable = node->isMutable();
  out->has_explicit_parameters = node->hasExplicitParameters();
  out->has_explicit_result_type = node->hasExplicitResultType();
  out->capture_default = enumValue<obj::LambdaCaptureDefault>(
      clava::LAMBDA_CAPTURE_DEFAULT[node->getCaptureDefault()]);
  out->lambda_class = wireId(clava::getId(node->getLambdaClass(), c.id));
  for (const auto capture : node->captures()) {
    out->capture_kinds.push_back(enumValue<obj::LambdaCaptureKind>(
        clava::LAMBDA_CAPTURE_KIND[capture.getCaptureKind()]));
  }
  return out;
}

std::unique_ptr<obj::PredefinedExprDataT> makePredefinedExprData(
    const clang::PredefinedExpr *node, Context &c) {
  auto out = std::make_unique<obj::PredefinedExprDataT>();
  out->base = makeExprData(node, c);
  out->predefined_type = enumValue<obj::PredefinedIdType>(
      clava::PREDEFINED_ID_TYPE[llvm::to_underlying(node->getIdentKind())]);
  return out;
}

std::unique_ptr<obj::SizeOfPackExprDataT> makeSizeOfPackExprData(
    const clang::SizeOfPackExpr *node, Context &c) {
  auto out = std::make_unique<obj::SizeOfPackExprDataT>();
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

std::unique_ptr<obj::ArrayInitLoopExprDataT> makeArrayInitLoopExprData(
    const clang::ArrayInitLoopExpr *node, Context &c) {
  auto out = std::make_unique<obj::ArrayInitLoopExprDataT>();
  out->base = makeExprData(node, c);
  return out;
}

std::unique_ptr<obj::DesignatedInitExprDataT> makeDesignatedInitExprData(
    const clang::DesignatedInitExpr *node, Context &c) {
  auto out = std::make_unique<obj::DesignatedInitExprDataT>();
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

std::unique_ptr<obj::DependentScopeDeclRefExprDataT>
makeDependentScopeDeclRefExprData(
    const clang::DependentScopeDeclRefExpr *node, Context &c) {
  auto out = std::make_unique<obj::DependentScopeDeclRefExprDataT>();
  out->base = makeExprData(node, c);
  out->decl_name = node->getDeclName().getAsString();
  out->qualifier = qualifierString(node->getQualifier(), c);
  out->has_template_keyword = node->hasTemplateKeyword();
  templateArguments(out->template_arguments, node, c);
  return out;
}

std::unique_ptr<obj::CXXNoexceptExprDataT> makeCXXNoexceptExprData(
    const clang::CXXNoexceptExpr *node, Context &c) {
  auto out = std::make_unique<obj::CXXNoexceptExprDataT>();
  out->base = makeExprData(node, c);
  out->value = node->getValue();
  return out;
}

std::unique_ptr<obj::CXXPseudoDestructorExprDataT>
makeCXXPseudoDestructorExprData(const clang::CXXPseudoDestructorExpr *node,
                                Context &c) {
  auto out = std::make_unique<obj::CXXPseudoDestructorExprDataT>();
  out->base = makeExprData(node, c);
  out->qualifier = node->hasQualifier() ? qualifierString(node->getQualifier(), c)
                                        : std::string();
  out->is_arrow = node->isArrow();
  out->destroyed_type = wireId(clava::getId(node->getDestroyedType(), c.id));
  return out;
}

std::unique_ptr<obj::PseudoObjectExprDataT> makePseudoObjectExprData(
    const clang::PseudoObjectExpr *node, Context &c) {
  auto out = std::make_unique<obj::PseudoObjectExprDataT>();
  out->base = makeExprData(node, c);
  auto index = node->getResultExprIndex();
  out->result_expr_index = index == clang::PseudoObjectExpr::NoResult
      ? -1 : static_cast<int>(index);
  return out;
}

std::unique_ptr<obj::MSPropertyRefExprDataT> makeMSPropertyRefExprData(
    const clang::MSPropertyRefExpr *node, Context &c) {
  auto out = std::make_unique<obj::MSPropertyRefExprDataT>();
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
std::unique_ptr<obj::CastExprDataT> makeCXXFunctionalCastExprData(
    const clang::CXXFunctionalCastExpr *node, Context &c) {
  return makeCastExprData(node, c);
}

std::unique_ptr<obj::ExplicitCastExprDataT> makeCStyleCastExprData(
    const clang::CStyleCastExpr *node, Context &c) {
  return makeExplicitCastExprData(node, c);
}

std::unique_ptr<obj::CXXNamedCastExprDataT> makeCXXAddrspaceCastExprData(
    const clang::CXXAddrspaceCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<obj::CXXNamedCastExprDataT> makeCXXConstCastExprData(
    const clang::CXXConstCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<obj::CXXNamedCastExprDataT> makeCXXDynamicCastExprData(
    const clang::CXXDynamicCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<obj::CXXNamedCastExprDataT> makeCXXReinterpretCastExprData(
    const clang::CXXReinterpretCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<obj::CXXNamedCastExprDataT> makeCXXStaticCastExprData(
    const clang::CXXStaticCastExpr *node, Context &c) {
  return makeCXXNamedCastExprData(node, c);
}

std::unique_ptr<obj::CallExprDataT> makeCXXOperatorCallExprData(
    const clang::CXXOperatorCallExpr *node, Context &c) {
  return makeCallExprData(node, c);
}

std::unique_ptr<obj::CallExprDataT> makeUserDefinedLiteralData(
    const clang::UserDefinedLiteral *node, Context &c) {
  return makeCallExprData(node, c);
}

} // namespace clava::proto
