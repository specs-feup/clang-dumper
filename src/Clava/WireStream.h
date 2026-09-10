#ifndef CLAVA_WIRE_STREAM_H
#define CLAVA_WIRE_STREAM_H

#include "llvm/Support/raw_ostream.h"

#include "wire_generated.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace clang {
class ASTContext;
class Expr;
class Stmt;
} // namespace clang

namespace clava {

class WireStream final : public llvm::raw_ostream {
  llvm::raw_ostream &Output;
  std::string Pending;
  std::unordered_map<std::string, uint32_t> Files;
  uint64_t RawBytes = 0;
  uint64_t TypedNodes = 0;
  uint64_t Records = 0;
  bool Finished = false;
  flatbuffers::FlatBufferBuilder Builder;

  void writeRecordHeader();
  void writeRecordRaw(const char *data, size_t size);
  void writeRecordFile(uint32_t id, llvm::StringRef path);
  void writeRecordNode(const clang::Stmt *stmt, clang::ASTContext *context,
                       int id);
  void writeRecordEnd();
  void flushRawChunks(bool all);

public:
  explicit WireStream(llvm::raw_ostream &Output);
  ~WireStream() override;

  bool emitNode(const clang::Stmt *stmt, clang::ASTContext *context, int id);
  uint32_t fileId(llvm::StringRef path);
  void finish();

  static WireStream *active();

protected:
  void write_impl(const char *Ptr, size_t Size) override;
  uint64_t current_pos() const override { return RawBytes + Pending.size(); }
};

namespace wire {

bool enabled();
bool emit(const clang::Stmt *stmt, clang::ASTContext *context, int id);
bool emit(const clang::Expr *expr, clang::ASTContext *context, int id);

} // namespace wire
} // namespace clava

#endif
