#include "WireStream.h"

#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"
#include "../ClavaDataDumper/ClavaDataDumper.h"

#include "wire_generated.h"

#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <system_error>

namespace {

constexpr size_t MaxRawChunkBytes = 64 * 1024;
clava::WireStream *ActiveWireStream = nullptr;

struct RangeValues {
  bool valid = false;
  uint32_t file = 0;
  uint32_t line = 0;
  uint32_t column = 0;
  uint32_t endFile = 0;
  uint32_t endLine = 0;
  uint32_t endColumn = 0;
};

struct SourceValues {
  RangeValues expansion;
  RangeValues spelling;
  bool isMacro = false;
  bool systemHeader = false;
};

struct TypedNode {
  uint32_t id = 0;
  astwire::flat::Kind kind = astwire::flat::Kind::INVALID;
  SourceValues source;
  bool isExpr = false;
  uint32_t typeId = 0;
  uint32_t valueKind = 0;
  uint32_t objectKind = 0;
  bool defaultArgument = false;
  std::string literalSource;
  std::string integerDecimal;
  std::string opcode;
  bool postfix = false;
  uint32_t declId = 0;
};

std::string sourcePath(const clang::SourceManager &sm,
                       clang::SourceLocation loc) {
  const auto filename = sm.getFilename(loc);
  return (filename.empty() ? sm.getBufferName(loc) : filename).str();
}

bool fillRange(RangeValues &out, const clang::SourceManager &sm,
               clang::SourceLocation begin, clang::SourceLocation end,
               clava::WireStream &stream) {
  const auto beginSpelling = sm.getSpellingLoc(begin);
  if (beginSpelling.isInvalid()) {
    return false;
  }

  out.valid = true;
  out.file = stream.fileId(sourcePath(sm, beginSpelling));
  out.line = sm.getSpellingLineNumber(beginSpelling);
  out.column = sm.getSpellingColumnNumber(beginSpelling);

  const auto endSpelling = sm.getSpellingLoc(end);
  if (begin == end || endSpelling.isInvalid()) {
    return true;
  }

  out.endFile = stream.fileId(sourcePath(sm, endSpelling));
  out.endLine = sm.getSpellingLineNumber(endSpelling);
  out.endColumn = sm.getSpellingColumnNumber(endSpelling);
  return true;
}

clang::SourceLocation resolveTokenSplitLocation(const clang::SourceManager &sm,
                                                 clang::SourceLocation loc) {
  while (loc.isMacroID()) {
    const unsigned offset = sm.getDecomposedLoc(loc).second;
    const auto expansion = sm.getImmediateExpansionRange(loc);
    if (expansion.isTokenRange()) {
      break;
    }
    const auto expanded = expansion.getBegin().getLocWithOffset(offset);
    if (expanded.isInvalid() || expanded == loc) {
      break;
    }
    loc = expanded;
  }
  return loc;
}

void fillSource(SourceValues &out, clang::ASTContext *context,
                clang::SourceLocation begin, clang::SourceLocation end,
                clava::WireStream &stream) {
  const auto &sm = context->getSourceManager();
  begin = resolveTokenSplitLocation(sm, begin);
  end = resolveTokenSplitLocation(sm, end);

  const auto expansion = sm.getExpansionRange(clang::SourceRange(begin, end));
  out.expansion = {};
  fillRange(out.expansion, sm, expansion.getBegin(), expansion.getEnd(),
            stream);

  out.isMacro = begin.isMacroID() || end.isMacroID();
  out.spelling = {};
  if (out.isMacro) {
    fillRange(out.spelling, sm, sm.getSpellingLoc(begin),
              sm.getSpellingLoc(end), stream);
  }

  const auto full = context->getFullLoc(begin);
  out.systemHeader = full.isValid() && full.isInSystemHeader();
}

uint32_t denseId(llvm::StringRef value) {
  if (!value.empty() && value.front() == '@') {
    uint32_t id = 0;
    const auto result = std::from_chars(value.data() + 1,
                                        value.data() + value.size(), id);
    if (result.ec == std::errc() && result.ptr == value.data() + value.size()) {
      return id;
    }
  }

  // Typed nullable fields use the legacy null sentinels or the void-pointer
  // spelling. Neither is a dense pointer ID.
  return 0;
}

void fillStmtBase(TypedNode &out, const clang::Stmt *stmt,
                  clang::ASTContext *context, clava::WireStream &stream,
                  int id, astwire::flat::Kind kind) {
  out.id = denseId(clava::getId(stmt, id));
  out.kind = kind;
  fillSource(out.source, context, stmt->getBeginLoc(), stmt->getEndLoc(),
             stream);
}

bool fillCommon(TypedNode &node, const clang::Stmt *stmt,
                clang::ASTContext *context, clava::WireStream &stream, int id,
                astwire::flat::Kind kind) {
  fillStmtBase(node, stmt, context, stream, id, kind);
  if (const auto *expr = llvm::dyn_cast<clang::Expr>(stmt)) {
    node.isExpr = true;
    node.typeId = denseId(clava::getId(expr->getType(), id));
    node.valueKind = static_cast<uint32_t>(expr->getValueKind());
    node.objectKind = static_cast<uint32_t>(expr->getObjectKind());
    node.defaultArgument = expr->isDefaultArgument();
    return true;
  }
  return false;
}

bool fillTyped(TypedNode &node, const clang::Stmt *stmt,
               clang::ASTContext *context, clava::WireStream &stream, int id) {
  const std::string className = clava::getClassName(stmt);
  const auto *expr = llvm::dyn_cast<clang::Expr>(stmt);

  astwire::flat::Kind kind;
  if (!expr) {
    if (className != "Stmt") {
      return false;
    }
    kind = astwire::flat::Kind::BASE_STMT;
  } else if (className == "Expr") {
    kind = astwire::flat::Kind::BASE_EXPR;
  } else if (className == "ImplicitCastExpr") {
    kind = astwire::flat::Kind::IMPLICIT_CAST;
  } else if (className == "IntegerLiteral") {
    kind = astwire::flat::Kind::INTEGER_LITERAL;
  } else if (className == "BinaryOperator") {
    kind = astwire::flat::Kind::BINARY_OPERATOR;
  } else if (className == "CompoundAssignOperator") {
    kind = astwire::flat::Kind::COMPOUND_ASSIGN;
  } else if (className == "UnaryOperator") {
    kind = astwire::flat::Kind::UNARY_OPERATOR;
  } else if (className == "CastExpr") {
    kind = astwire::flat::Kind::CAST_EXPR;
  } else if (className == "CXXFunctionalCastExpr") {
    kind = astwire::flat::Kind::FUNCTIONAL_CAST;
  } else if (className == "DeclRefExpr") {
    const auto *ref = llvm::cast<clang::DeclRefExpr>(expr);
    if (ref->hasExplicitTemplateArgs() || ref->getQualifier() != nullptr) {
      return false;
    }
    kind = astwire::flat::Kind::DECL_REF;
  } else {
    return false;
  }

  const bool isExpr = fillCommon(node, stmt, context, stream, id, kind);
  if (!isExpr) {
    return true;
  }

  if (const auto *literal = llvm::dyn_cast<clang::IntegerLiteral>(expr)) {
    node.literalSource =
        clava::getSourceText(context, literal->getSourceRange());
    llvm::SmallString<0> decimal;
    literal->getValue().toString(decimal, 10,
                                 literal->getType()->isSignedIntegerType());
    node.integerDecimal = decimal.str().str();
    return true;
  }
  if (const auto *binary = llvm::dyn_cast<clang::BinaryOperator>(expr)) {
    node.opcode = clava::BINARY_OPERATOR_KIND[binary->getOpcode()];
    return true;
  }
  if (const auto *unary = llvm::dyn_cast<clang::UnaryOperator>(expr)) {
    node.opcode = clava::UNARY_OPERATOR_KIND[unary->getOpcode()];
    node.postfix = unary->isPostfix();
    return true;
  }
  if ((className == "CastExpr" || className == "CXXFunctionalCastExpr") &&
      llvm::isa<clang::CastExpr>(expr)) {
    const auto *cast = llvm::cast<clang::CastExpr>(expr);
    node.opcode = clava::CAST_KIND[cast->getCastKind()];
    return true;
  }
  if (const auto *ref = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
    node.declId = denseId(clava::getId(ref->getDecl(), id));
    return true;
  }
  return className == "Expr" || className == "ImplicitCastExpr";
}

bool isTypedCandidate(const clang::Stmt *stmt) {
  const std::string className = clava::getClassName(stmt);
  if (className == "Stmt" || className == "Expr" ||
      className == "ImplicitCastExpr" || className == "IntegerLiteral" ||
      className == "BinaryOperator" ||
      className == "CompoundAssignOperator" || className == "UnaryOperator" ||
      className == "CastExpr" || className == "CXXFunctionalCastExpr") {
    return true;
  }
  if (className != "DeclRefExpr") {
    return false;
  }

  const auto *ref = llvm::cast<clang::DeclRefExpr>(stmt);
  return !ref->hasExplicitTemplateArgs() && ref->getQualifier() == nullptr;
}

flatbuffers::Offset<astwire::flat::Range>
createRange(flatbuffers::FlatBufferBuilder &builder, const RangeValues &range) {
  if (!range.valid) {
    return 0;
  }
  return astwire::flat::CreateRange(builder, range.file, range.line,
                                    range.column, range.endFile,
                                    range.endLine, range.endColumn);
}

flatbuffers::Offset<astwire::flat::Node>
createNode(flatbuffers::FlatBufferBuilder &builder, const TypedNode &node) {
  const auto expansion = createRange(builder, node.source.expansion);
  const auto spelling = createRange(builder, node.source.spelling);

  flatbuffers::Offset<void> payload;
  astwire::flat::NodePayload payloadType = astwire::flat::NodePayload::NONE;

  if (node.kind == astwire::flat::Kind::BASE_STMT) {
    payload = astwire::flat::CreateBaseStmt(builder).Union();
    payloadType = astwire::flat::NodePayload::BaseStmt;
  } else if (node.kind == astwire::flat::Kind::BASE_EXPR ||
             node.kind == astwire::flat::Kind::IMPLICIT_CAST) {
    payload = astwire::flat::CreateBaseExpr(builder).Union();
    payloadType = astwire::flat::NodePayload::BaseExpr;
  } else if (node.kind == astwire::flat::Kind::INTEGER_LITERAL) {
    const auto literal = builder.CreateVector(
        reinterpret_cast<const uint8_t *>(node.literalSource.data()),
        node.literalSource.size());
    const auto decimal = builder.CreateVector(
        reinterpret_cast<const uint8_t *>(node.integerDecimal.data()),
        node.integerDecimal.size());
    payload = astwire::flat::CreateIntegerLiteral(builder, literal, decimal)
                  .Union();
    payloadType = astwire::flat::NodePayload::IntegerLiteral;
  } else if (node.kind == astwire::flat::Kind::BINARY_OPERATOR) {
    const auto opcode = builder.CreateVector(
        reinterpret_cast<const uint8_t *>(node.opcode.data()),
        node.opcode.size());
    payload = astwire::flat::CreateBinaryOperator(builder, opcode).Union();
    payloadType = astwire::flat::NodePayload::BinaryOperator;
  } else if (node.kind == astwire::flat::Kind::COMPOUND_ASSIGN) {
    const auto opcode = builder.CreateVector(
        reinterpret_cast<const uint8_t *>(node.opcode.data()),
        node.opcode.size());
    payload = astwire::flat::CreateCompoundAssign(builder, opcode).Union();
    payloadType = astwire::flat::NodePayload::CompoundAssign;
  } else if (node.kind == astwire::flat::Kind::UNARY_OPERATOR) {
    const auto opcode = builder.CreateVector(
        reinterpret_cast<const uint8_t *>(node.opcode.data()),
        node.opcode.size());
    payload = astwire::flat::CreateUnaryOperator(builder, opcode, node.postfix)
                  .Union();
    payloadType = astwire::flat::NodePayload::UnaryOperator;
  } else if (node.kind == astwire::flat::Kind::CAST_EXPR) {
    const auto opcode = builder.CreateVector(
        reinterpret_cast<const uint8_t *>(node.opcode.data()),
        node.opcode.size());
    payload = astwire::flat::CreateCastExpr(builder, opcode).Union();
    payloadType = astwire::flat::NodePayload::CastExpr;
  } else if (node.kind == astwire::flat::Kind::FUNCTIONAL_CAST) {
    const auto opcode = builder.CreateVector(
        reinterpret_cast<const uint8_t *>(node.opcode.data()),
        node.opcode.size());
    payload = astwire::flat::CreateFunctionalCast(builder, opcode).Union();
    payloadType = astwire::flat::NodePayload::FunctionalCast;
  } else if (node.kind == astwire::flat::Kind::DECL_REF) {
    payload = astwire::flat::CreateDeclRef(builder, node.declId).Union();
    payloadType = astwire::flat::NodePayload::DeclRef;
  }

  return astwire::flat::CreateNode(
      builder, node.id, node.kind, expansion, spelling, node.source.isMacro,
      node.source.systemHeader, node.typeId, node.valueKind, node.objectKind,
      node.defaultArgument, payloadType, payload);
}

} // namespace

clava::WireStream::WireStream(llvm::raw_ostream &Output) : Output(Output) {
  ActiveWireStream = this;
  writeRecordHeader();
}

clava::WireStream::~WireStream() {
  if (!Finished) {
    finish();
  }
  if (ActiveWireStream == this) {
    ActiveWireStream = nullptr;
  }
}

void clava::WireStream::write_impl(const char *Ptr, size_t Size) {
  Pending.append(Ptr, Size);
  flushRawChunks(false);
}

void clava::WireStream::writeRecordHeader() {
  Builder.Clear();
  const auto schema = Builder.CreateString("ast-wire-flat-v1");
  const auto body = astwire::flat::CreateHeader(Builder, schema);
  const auto record = astwire::flat::CreateRecord(
      Builder, astwire::flat::RecordBody::Header, body.Union());
  astwire::flat::FinishSizePrefixedRecordBuffer(Builder, record);
  Output.write(reinterpret_cast<const char *>(Builder.GetBufferPointer()),
               Builder.GetSize());
  ++Records;
}

void clava::WireStream::writeRecordRaw(const char *data, size_t size) {
  Builder.Clear();
  const auto bytes = Builder.CreateVector(
      reinterpret_cast<const uint8_t *>(data), size);
  const auto body = astwire::flat::CreateRaw(Builder, bytes);
  const auto record = astwire::flat::CreateRecord(
      Builder, astwire::flat::RecordBody::Raw, body.Union());
  astwire::flat::FinishSizePrefixedRecordBuffer(Builder, record);
  Output.write(reinterpret_cast<const char *>(Builder.GetBufferPointer()),
               Builder.GetSize());
  ++Records;
}

void clava::WireStream::writeRecordFile(uint32_t id, llvm::StringRef path) {
  Builder.Clear();
  const auto bytes = Builder.CreateVector(
      reinterpret_cast<const uint8_t *>(path.data()), path.size());
  const auto body = astwire::flat::CreateFile(Builder, id, bytes);
  const auto record = astwire::flat::CreateRecord(
      Builder, astwire::flat::RecordBody::File, body.Union());
  astwire::flat::FinishSizePrefixedRecordBuffer(Builder, record);
  Output.write(reinterpret_cast<const char *>(Builder.GetBufferPointer()),
               Builder.GetSize());
  ++Records;
}

void clava::WireStream::writeRecordNode(const clang::Stmt *stmt,
                                        clang::ASTContext *context, int id) {
  TypedNode node;
  if (!fillTyped(node, stmt, context, *this, id)) {
    return;
  }

  Builder.Clear();
  const auto body = createNode(Builder, node);
  const auto record = astwire::flat::CreateRecord(
      Builder, astwire::flat::RecordBody::Node, body.Union());
  astwire::flat::FinishSizePrefixedRecordBuffer(Builder, record);
  Output.write(reinterpret_cast<const char *>(Builder.GetBufferPointer()),
               Builder.GetSize());
  ++Records;
  ++TypedNodes;
}

void clava::WireStream::writeRecordEnd() {
  Builder.Clear();
  const auto body = astwire::flat::CreateEnd(
      Builder, Records + 1, TypedNodes, RawBytes,
      static_cast<uint32_t>(Files.size()),
      static_cast<uint32_t>(clava::denseIdCount()));
  const auto record = astwire::flat::CreateRecord(
      Builder, astwire::flat::RecordBody::End, body.Union());
  astwire::flat::FinishSizePrefixedRecordBuffer(Builder, record);
  Output.write(reinterpret_cast<const char *>(Builder.GetBufferPointer()),
               Builder.GetSize());
  ++Records;
}

void clava::WireStream::flushRawChunks(bool all) {
  while (Pending.size() >= MaxRawChunkBytes || (all && !Pending.empty())) {
    const size_t size = all ? std::min(Pending.size(), MaxRawChunkBytes)
                            : MaxRawChunkBytes;
    writeRecordRaw(Pending.data(), size);
    Pending.erase(0, size);
    RawBytes += size;
  }
}

uint32_t clava::WireStream::fileId(llvm::StringRef path) {
  const std::string key = path.str();
  const auto found = Files.find(key);
  if (found != Files.end()) {
    return found->second;
  }

  // File records can be inserted between raw chunks. Drain pending bytes
  // first so they never move ahead of the raw prefix they came from.
  llvm::raw_ostream::flush();
  flushRawChunks(true);

  const uint32_t id = static_cast<uint32_t>(Files.size() + 1);
  Files.emplace(key, id);
  writeRecordFile(id, key);
  return id;
}

bool clava::WireStream::emitNode(const clang::Stmt *stmt,
                                 clang::ASTContext *context, int id) {
  if (!isTypedCandidate(stmt)) {
    return false;
  }
  // raw_ostream keeps a small front buffer before calling write_impl(). Drain
  // it first so fallback text cannot leapfrog a typed record.
  llvm::raw_ostream::flush();
  flushRawChunks(true);
  const uint64_t before = TypedNodes;
  writeRecordNode(stmt, context, id);
  return TypedNodes != before;
}

void clava::WireStream::finish() {
  if (Finished) {
    return;
  }
  llvm::raw_ostream::flush();
  flushRawChunks(true);
  writeRecordEnd();
  Output.flush();
  Finished = true;
}

clava::WireStream *clava::WireStream::active() { return ActiveWireStream; }

bool clava::wire::enabled() { return clava::WireStream::active() != nullptr; }

bool clava::wire::emit(const clang::Stmt *stmt, clang::ASTContext *context,
                       int id) {
  auto *stream = clava::WireStream::active();
  if (!stream) {
    return false;
  }
  return stream->emitNode(stmt, context, id);
}

bool clava::wire::emit(const clang::Expr *expr, clang::ASTContext *context,
                       int id) {
  return emit(static_cast<const clang::Stmt *>(expr), context, id);
}
