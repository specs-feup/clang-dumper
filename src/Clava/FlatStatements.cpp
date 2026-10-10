#include "FlatSupport.h"

#include "clang/AST/Expr.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtObjC.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"

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
  out->asm_string = node->getAsmString()->getString().str();
  out->is_goto = node->isAsmGoto();
  for (unsigned i = 0; i < node->getNumLabels(); ++i) {
    out->labels.push_back(node->getLabelName(i).str());
  }
  out->is_inline = c.isInlineAsm(node->getBeginLoc());
  return out;
}

namespace {

std::string getSourceMSAsmString(const clang::MSAsmStmt *node, Context &c) {
  const auto &sourceManager = c.ast->getSourceManager();
  const auto &langOptions = c.ast->getLangOpts();

  clang::SourceLocation begin;
  clang::SourceLocation end;
  if (node->hasBraces()) {
    begin = sourceManager.getSpellingLoc(node->getLBraceLoc())
                .getLocWithOffset(1);
    end = sourceManager.getSpellingLoc(node->getEndLoc());
  } else {
    // Single-line MS asm has no brace range, so use the first and last retained
    // source tokens to recover its original spelling.
    auto *mutableNode = const_cast<clang::MSAsmStmt *>(node);
    const unsigned count = mutableNode->getNumAsmToks();
    auto *tokens = mutableNode->getAsmToks();
    if (count != 0) {
      begin = sourceManager.getSpellingLoc(tokens[0].getLocation());
      end = clang::Lexer::getLocForEndOfToken(
          sourceManager.getSpellingLoc(tokens[count - 1].getLocation()), 0,
          sourceManager, langOptions);
    }
  }

  if (begin.isValid() && end.isValid()) {
    bool invalid = false;
    const auto source = clang::Lexer::getSourceText(
        clang::CharSourceRange::getCharRange(begin, end), sourceManager,
        langOptions, &invalid);
    if (!invalid) {
      return source.str();
    }
  }

  // Macro-backed asm can have no contiguous source range. Rebuild from Clang's
  // retained source tokens rather than its IR-normalized asm string.
  auto *mutableNode = const_cast<clang::MSAsmStmt *>(node);
  const unsigned count = mutableNode->getNumAsmToks();
  auto *tokens = mutableNode->getAsmToks();
  std::string result;
  for (unsigned i = 0; i < count; ++i) {
    const auto &token = tokens[i];
    if (i != 0) {
      if (token.isAtStartOfLine()) {
        result += "\n";
      } else if (token.hasLeadingSpace()) {
        result += ' ';
      }
    }
    result += clang::Lexer::getSpelling(token, sourceManager, langOptions);
  }
  return result;
}

} // namespace

std::unique_ptr<fb::MSAsmStmtDataT> makeMSAsmStmtData(
    const clang::MSAsmStmt *node, Context &c) {
  auto out = std::make_unique<fb::MSAsmStmtDataT>();
  out->base = makeAsmStmtData(node, c);
  out->asm_string = getSourceMSAsmString(node, c);
  return out;
}

/** Payload builder per emitted Clang class; other classes use the base Stmt payload. */
fb::NodeT makeNode(const clang::Stmt *node, Context &c) {
  auto out = nodeHeader(node, c);
  FLAT_PAYLOAD(LabelStmt, makeLabelStmtData)
  FLAT_PAYLOAD(GotoStmt, makeGotoStmtData)
  FLAT_PAYLOAD(AttributedStmt, makeAttributedStmtData)
  FLAT_PAYLOAD(GCCAsmStmt, makeGCCAsmStmtData)
  FLAT_PAYLOAD(MSAsmStmt, makeMSAsmStmtData)
  out.payload.Set(std::move(*makeStmtData(node, c)));
  return out;
}

} // namespace clava::flat
