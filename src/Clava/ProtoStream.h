#pragma once

#include "ProtoSupport.h"
#include "ProtoEncode.h"
#include "HandlerCoverage.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

namespace clang {
class ASTContext;
class Attr;
class Decl;
class Expr;
class QualType;
class Stmt;
class Type;
} // namespace clang

namespace clava::proto {

#include "ProtoDispatch.inc"

class ProtoStream final : public llvm::raw_ostream {
  static constexpr size_t ChunkTargetBytes = 64 * 1024;
  static constexpr size_t FlushThreshold = 64 * 1024;
  static constexpr size_t MaxFrameBytes = 64 * 1024 * 1024;

  llvm::raw_ostream &output;
  std::string pending;
  pb::Chunk pending_chunk;
  std::unordered_map<std::string, uint32_t> files;
  uint64_t bytes_written = 0;
  uint64_t records = 0;
  uint64_t nodes = 0;
  bool finished = false;

  void writeHeader();
  void writeEnvelope(const pb::Envelope &envelope);
  void writeChunk();
  void appendRecord(pb::Record record);
  void flushPending();
  uint32_t fileId(llvm::StringRef path);

  void write_impl(const char *data, size_t size) override;
  uint64_t current_pos() const override {
    return bytes_written + pending.size();
  }

public:
  explicit ProtoStream(llvm::raw_ostream &output);
  ~ProtoStream() override;

  static ProtoStream *active();

  template <typename T> void record(const T &value) {
    pb::Record record;
    setRecord(value, &record);
    appendRecord(std::move(record));
    ++records;
  }

  template <typename T> void node(const T *node, clang::ASTContext *ast,
                                  int id) {
    Context context{ast, id, [this](llvm::StringRef path) {
                      return fileId(path);
                    }};
    record(makeNode(node, context));
    ++nodes;
  }

  void node(const clang::QualType &node, clang::ASTContext *ast, int id) {
    Context context{ast, id, [this](llvm::StringRef path) {
                      return fileId(path);
                    }};
    record(makeNode(node, context));
    ++nodes;
  }

  void finish();
};

bool enabled();
bool emit(const clang::Decl *node, clang::ASTContext *ast, int id);
bool emit(const clang::Stmt *node, clang::ASTContext *ast, int id);
bool emit(const clang::Expr *node, clang::ASTContext *ast, int id);
bool emit(const clang::Type *node, clang::ASTContext *ast, int id);
bool emit(const clang::QualType &node, clang::ASTContext *ast, int id);
bool emit(const clang::Attr *node, clang::ASTContext *ast, int id);

} // namespace clava::proto
