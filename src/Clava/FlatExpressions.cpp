#include "FlatSupport.h"

#include "../ClangEnums/ClangEnums.h"

#include "clang/AST/Decl.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/StmtCXX.h"
#include "llvm/ADT/STLForwardCompat.h"
#include "llvm/Support/Casting.h"

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
  case clang::OK_MatrixComponent:
    return fb::ObjectKind::MATRIX_COMPONENT;
  }
  throw std::invalid_argument("Unsupported Clang expression object kind");
}

using TemplateArguments = std::vector<std::unique_ptr<fb::TemplateArgumentT>>;

template <typename Node> TemplateArguments templateArguments(const Node *node, Context &c) {
  TemplateArguments out;
  if (node->hasExplicitTemplateArgs()) {
    const auto *args = node->getTemplateArgs();
    for (unsigned i = 0; i < node->getNumTemplateArgs(); ++i)
      out.push_back(makeTemplateArgument((args + i)->getArgument(), c));
  }
  return out;
}

TemplateArguments partialArguments(const clang::SizeOfPackExpr *node, Context &c) {
  TemplateArguments out;
  if (node->isPartiallySubstituted())
    for (const auto &arg : node->getPartialArguments())
      out.push_back(makeTemplateArgument(arg, c));
  return out;
}

std::string integerText(const clang::IntegerLiteral *node) {
  llvm::SmallString<0> value;
  node->getValue().toString(value, 10, node->getType()->isSignedIntegerType());
  return value.str().str();
}

// Preserve the decimal precision expected by existing Clava consumers; LiteralData keeps the source spelling.
double approximateDouble(const clang::FloatingLiteral *node) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  stream << node->getValueAsApproximateDouble();
  return std::stod(text);
}

std::vector<uint8_t> stringBytes(const clang::StringLiteral *node) {
  const auto bytes = node->getBytes();
  return {bytes.bytes_begin(), bytes.bytes_end()};
}

fb::UnaryOperatorPosition position(const clang::UnaryOperator *node) {
  return enumValue<fb::UnaryOperatorPosition>(node->isPostfix() ? "POSTFIX" : "PREFIX");
}

std::vector<std::unique_ptr<fb::OffsetOfComponentT>> offsetOfComponents(const clang::OffsetOfExpr *node,
                                                                       Context &c) {
  std::vector<std::unique_ptr<fb::OffsetOfComponentT>> out;
  for (unsigned i = 0; i < node->getNumComponents(); ++i)
    out.push_back(makeOffsetOfComponent(node, i, c));
  return out;
}

std::vector<std::unique_ptr<fb::DesignatorT>> designators(const clang::DesignatedInitExpr *node, Context &c) {
  std::vector<std::unique_ptr<fb::DesignatorT>> out;
  for (unsigned i = 0; i < node->size(); ++i) {
    auto *designator = node->getDesignator(i);
    if (!designator->isFieldDesignator() || designator->getFieldName() != nullptr)
      out.push_back(makeDesignator(designator, c));
  }
  return out;
}

int32_t resultExprIndex(const clang::PseudoObjectExpr *node) {
  auto index = node->getResultExprIndex();
  return index == clang::PseudoObjectExpr::NoResult ? -1 : static_cast<int32_t>(index);
}

} // namespace

#include "expressions_builders.inc"

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
  out->lambda_class = clava::getId(node->getLambdaClass(), c.id);
  for (const auto &capture : node->captures()) {
    out->capture_kinds.push_back(enumValue<fb::LambdaCaptureKind>(
        clava::LAMBDA_CAPTURE_KIND[capture.getCaptureKind()]));
    const bool isInitCapture = node->isInitCapture(&capture);
    out->init_capture_names.push_back(isInitCapture
                                          ? capture.getCapturedVar()->getNameAsString()
                                          : "");
    const auto *capturedVar = isInitCapture
                                  ? llvm::dyn_cast<clang::VarDecl>(
                                        capture.getCapturedVar())
                                  : nullptr;
    if (isInitCapture && !capturedVar) {
      throw std::invalid_argument(
          "An init-capture must reference a VarDecl");
    }
    const auto initStyle = capturedVar ? capturedVar->getInitStyle()
                                       : clang::VarDecl::CInit;
    out->capture_init_styles.push_back(enumValue<fb::InitializationStyle>(
        clava::INIT_STYLE[initStyle]));
    // Init-capture packs put the ellipsis before the new variable name;
    // LambdaCapture::isPackExpansion() describes trailing capture ellipses.
    out->capture_pack_expansions.push_back(
        capture.isPackExpansion() ||
        (capturedVar && capturedVar->isParameterPack()));
    out->capture_is_implicit.push_back(capture.isImplicit());
  }
  return out;
}

/** Payload builder per emitted Clang class; other classes use the base Expr payload. */
fb::NodeT makeNode(const clang::Expr *node, Context &c) {
  auto out = nodeHeader(node, c);
  FLAT_PAYLOAD(CharacterLiteral, makeCharacterLiteralData)
  FLAT_PAYLOAD(IntegerLiteral, makeIntegerLiteralData)
  FLAT_PAYLOAD(FloatingLiteral, makeFloatingLiteralData)
  FLAT_PAYLOAD(CastExpr, makeCastExprData)
  FLAT_PAYLOAD(CXXUnresolvedConstructExpr, makeCXXUnresolvedConstructExprData)
  FLAT_PAYLOAD(CXXFunctionalCastExpr, makeCastExprData)
  FLAT_PAYLOAD(CStyleCastExpr, makeExplicitCastExprData)
  FLAT_PAYLOAD(CXXAddrspaceCastExpr, makeCXXNamedCastExprData)
  FLAT_PAYLOAD(CXXConstCastExpr, makeCXXNamedCastExprData)
  FLAT_PAYLOAD(CXXDynamicCastExpr, makeCXXNamedCastExprData)
  FLAT_PAYLOAD(CXXReinterpretCastExpr, makeCXXNamedCastExprData)
  FLAT_PAYLOAD(CXXStaticCastExpr, makeCXXNamedCastExprData)
  FLAT_PAYLOAD(CXXBoolLiteralExpr, makeCXXBoolLiteralExprData)
  FLAT_PAYLOAD(CompoundLiteralExpr, makeCompoundLiteralExprData)
  FLAT_PAYLOAD(InitListExpr, makeInitListExprData)
  FLAT_PAYLOAD(StringLiteral, makeStringLiteralData)
  FLAT_PAYLOAD(DeclRefExpr, makeDeclRefExprData)
  FLAT_PAYLOAD(DependentScopeDeclRefExpr, makeDependentScopeDeclRefExprData)
  FLAT_PAYLOAD(UnresolvedLookupExpr, makeUnresolvedLookupExprData)
  FLAT_PAYLOAD(UnresolvedMemberExpr, makeUnresolvedMemberExprData)
  FLAT_PAYLOAD(CXXConstructExpr, makeCXXConstructExprData)
  FLAT_PAYLOAD(CXXTemporaryObjectExpr, makeCXXTemporaryObjectExprData)
  FLAT_PAYLOAD(MemberExpr, makeMemberExprData)
  FLAT_PAYLOAD(MaterializeTemporaryExpr, makeMaterializeTemporaryExprData)
  FLAT_PAYLOAD(BinaryOperator, makeBinaryOperatorData)
  FLAT_PAYLOAD(UnaryOperator, makeUnaryOperatorData)
  FLAT_PAYLOAD(CompoundAssignOperator, makeBinaryOperatorData)
  FLAT_PAYLOAD(CallExpr, makeCallExprData)
  FLAT_PAYLOAD(CXXMemberCallExpr, makeCXXMemberCallExprData)
  FLAT_PAYLOAD(CXXOperatorCallExpr, makeCallExprData)
  FLAT_PAYLOAD(UserDefinedLiteral, makeCallExprData)
  FLAT_PAYLOAD(CXXTypeidExpr, makeCXXTypeidExprData)
  FLAT_PAYLOAD(CXXDependentScopeMemberExpr, makeCXXDependentScopeMemberExprData)
  FLAT_PAYLOAD(UnaryExprOrTypeTraitExpr, makeUnaryExprOrTypeTraitExprData)
  FLAT_PAYLOAD(CXXNewExpr, makeCXXNewExprData)
  FLAT_PAYLOAD(CXXDeleteExpr, makeCXXDeleteExprData)
  FLAT_PAYLOAD(OffsetOfExpr, makeOffsetOfExprData)
  FLAT_PAYLOAD(LambdaExpr, makeLambdaExprData)
  FLAT_PAYLOAD(PredefinedExpr, makePredefinedExprData)
  FLAT_PAYLOAD(SizeOfPackExpr, makeSizeOfPackExprData)
  FLAT_PAYLOAD(ArrayInitLoopExpr, makeArrayInitLoopExprData)
  FLAT_PAYLOAD(DesignatedInitExpr, makeDesignatedInitExprData)
  FLAT_PAYLOAD(CXXNoexceptExpr, makeCXXNoexceptExprData)
  FLAT_PAYLOAD(CXXPseudoDestructorExpr, makeCXXPseudoDestructorExprData)
  FLAT_PAYLOAD(PseudoObjectExpr, makePseudoObjectExprData)
  FLAT_PAYLOAD(MSPropertyRefExpr, makeMSPropertyRefExprData)
  out.payload.Set(std::move(*makeExprData(node, c)));
  return out;
}

} // namespace clava::flat
