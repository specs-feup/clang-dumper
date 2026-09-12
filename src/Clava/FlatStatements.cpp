#include "FlatSupport.h"

#include "clang/AST/Expr.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtObjC.h"

namespace clava::flat {

std::unique_ptr<fb::StmtDataT> makeStmtData(const clang::Stmt *node,
                                            Context &c) {
  auto out = std::make_unique<fb::StmtDataT>();
  out->base = makeNodeData(node->getBeginLoc(), node->getEndLoc(), c);
  return out;
}

std::unique_ptr<fb::LabelStmtDataT> makeLabelStmtData(
    const clang::LabelStmt *node, Context &c) {
  auto out = std::make_unique<fb::LabelStmtDataT>();
  out->base = makeStmtData(node, c);
  out->label = wireId(clava::getId(node->getDecl(), c.id));
  return out;
}

std::unique_ptr<fb::GotoStmtDataT> makeGotoStmtData(
    const clang::GotoStmt *node, Context &c) {
  auto out = std::make_unique<fb::GotoStmtDataT>();
  out->base = makeStmtData(node, c);
  out->label = wireId(clava::getId(node->getLabel(), c.id));
  return out;
}

std::unique_ptr<fb::AttributedStmtDataT> makeAttributedStmtData(
    const clang::AttributedStmt *node, Context &c) {
  auto out = std::make_unique<fb::AttributedStmtDataT>();
  out->base = makeStmtData(node, c);
  for (const auto *attr : node->getAttrs()) {
    out->stmt_attributes.push_back(wireId(clava::getId(attr, c.id)));
  }
  return out;
}

std::unique_ptr<fb::AsmStmtDataT> makeAsmStmtData(const clang::AsmStmt *node,
                                                  Context &c) {
  auto out = std::make_unique<fb::AsmStmtDataT>();
  out->base = makeStmtData(node, c);
  out->is_simple = node->isSimple();
  out->is_volatile = node->isVolatile();

  for (unsigned i = 0; i < node->getNumClobbers(); ++i) {
    out->clobbers.emplace_back(node->getClobber(i).str());
  }
  for (unsigned i = 0; i < node->getNumOutputs(); ++i) {
    auto output = std::make_unique<fb::AsmOutputT>();
    output->expr = wireId(clava::getId(node->getOutputExpr(i), c.id));
    output->constraint = node->getOutputConstraint(i).str();
    output->is_plus_constraint = node->isOutputPlusConstraint(i);
    out->outputs.push_back(std::move(output));
  }
  for (unsigned i = 0; i < node->getNumInputs(); ++i) {
    auto input = std::make_unique<fb::AsmInputT>();
    input->expr = wireId(clava::getId(node->getInputExpr(i), c.id));
    input->constraint = node->getInputConstraint(i).str();
    out->inputs.push_back(std::move(input));
  }
  return out;
}

std::unique_ptr<fb::GCCAsmStmtDataT> makeGCCAsmStmtData(
    const clang::GCCAsmStmt *node, Context &c) {
  auto out = std::make_unique<fb::GCCAsmStmtDataT>();
  out->base = makeAsmStmtData(node, c);
  out->asm_string = node->generateAsmString(*c.ast);
  return out;
}

std::unique_ptr<fb::MSAsmStmtDataT> makeMSAsmStmtData(
    const clang::MSAsmStmt *node, Context &c) {
  auto out = std::make_unique<fb::MSAsmStmtDataT>();
  out->base = makeAsmStmtData(node, c);
  out->asm_string = node->generateAsmString(*c.ast);
  return out;
}

} // namespace clava::flat
