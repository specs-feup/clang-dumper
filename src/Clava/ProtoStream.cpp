#include "ProtoStream.h"

#include "ProtoSchemaHash.h"
#include "../Clang/ClangNodes.h"

#include <array>
#include <stdexcept>

namespace {
clava::proto::ProtoStream *ActiveStream = nullptr;
constexpr std::array<char, 8> Magic{{'C', 'L', 'A', 'V', 'A', 'P', 'B', '1'}};

void appendVarint(std::string &target, uint64_t value) {
  while (value >= 0x80) {
    target.push_back(static_cast<char>((value & 0x7f) | 0x80));
    value >>= 7;
  }
  target.push_back(static_cast<char>(value));
}
} // namespace

clava::proto::ProtoStream::ProtoStream(llvm::raw_ostream &output)
    : output(output) {
  if (ActiveStream != nullptr)
    throw std::logic_error("Nested protobuf dump streams");

  SetUnbuffered();
  ActiveStream = this;
  output.write(Magic.data(), Magic.size());
  bytes_written = Magic.size();
  writeHeader();
  flushPending();
}

clava::proto::ProtoStream::~ProtoStream() {
  if (!finished)
    finish();
  if (ActiveStream == this)
    ActiveStream = nullptr;
}

void clava::proto::ProtoStream::write_impl(const char *, size_t size) {
  if (size != 0)
    throw std::logic_error("Raw bytes reached the protobuf AST writer");
}

void clava::proto::ProtoStream::writeHeader() {
  pb::Envelope envelope;
  auto *header = envelope.mutable_header();
  header->set_protocol_major(1);
  header->set_protocol_minor(0);
  header->set_schema_id("clava-ast-wire");
  header->set_producer_version("clang-dumper-18");
  header->set_llvm_major(18);
  header->set_schema_sha256(clava::proto::ProtoSchemaHash);
  writeEnvelope(envelope);
  ++records;
}

void clava::proto::ProtoStream::writeEnvelope(const pb::Envelope &envelope) {
  const size_t payload_size = envelope.ByteSizeLong();
  if (payload_size == 0)
    throw std::logic_error("Cannot write an empty protobuf AST frame");
  if (payload_size > MaxFrameBytes)
    throw std::length_error("Protobuf AST frame exceeds 64 MiB limit");

  std::string payload;
  payload.reserve(payload_size);
  if (!envelope.SerializeToString(&payload))
    throw std::runtime_error("Cannot serialize protobuf AST record");

  appendVarint(pending, payload.size());
  pending.append(payload);
  if (pending.size() >= FlushThreshold)
    flushPending();
}

void clava::proto::ProtoStream::appendRecord(pb::Record record) {
  const size_t record_size = record.ByteSizeLong();
  if (record_size > MaxFrameBytes)
    throw std::length_error("Protobuf AST record exceeds 64 MiB limit");

  if (!pending_chunk.records().empty() &&
      pending_chunk.ByteSizeLong() + record_size > ChunkTargetBytes)
    writeChunk();

  *pending_chunk.add_records() = std::move(record);
  if (pending_chunk.ByteSizeLong() >= ChunkTargetBytes)
    writeChunk();
}

void clava::proto::ProtoStream::writeChunk() {
  if (pending_chunk.records().empty())
    return;

  pb::Envelope envelope;
  *envelope.mutable_chunk() = std::move(pending_chunk);
  pending_chunk.Clear();
  writeEnvelope(envelope);
  flushPending();
}

void clava::proto::ProtoStream::flushPending() {
  if (pending.empty())
    return;
  output.write(pending.data(), pending.size());
  bytes_written += pending.size();
  pending.clear();
  output.flush();
}

uint32_t clava::proto::ProtoStream::fileId(llvm::StringRef path) {
  auto [entry, inserted] =
      files.try_emplace(path.str(), static_cast<uint32_t>(files.size() + 1));
  if (inserted) {
    obj::FileT file;
    file.id = entry->second;
    file.path = entry->first;
    record(file);
  }
  return entry->second;
}

void clava::proto::ProtoStream::finish() {
  if (finished)
    return;

  flush();
  writeChunk();
  flushPending();

  pb::Envelope envelope;
  auto *end = envelope.mutable_end();
  end->set_records(records);
  end->set_nodes(nodes);
  end->set_raw_bytes(bytes_written + pending.size());
  end->set_files(files.size());
  end->set_ids(clava::denseIdCount());
  writeEnvelope(envelope);
  ++records;
  flushPending();
  output.flush();
  finished = true;
}

clava::proto::ProtoStream *clava::proto::ProtoStream::active() {
  return ActiveStream;
}

bool clava::proto::enabled() { return ProtoStream::active() != nullptr; }

#define CLAVA_PROTO_EMIT(TYPE)                                                \
  bool clava::proto::emit(const clang::TYPE *node, clang::ASTContext *ast,     \
                          int id) {                                           \
    if (auto *stream = ProtoStream::active()) {                               \
      stream->node(node, ast, id);                                             \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }

CLAVA_PROTO_EMIT(Decl)
CLAVA_PROTO_EMIT(Stmt)
CLAVA_PROTO_EMIT(Expr)
CLAVA_PROTO_EMIT(Type)
CLAVA_PROTO_EMIT(Attr)

bool clava::proto::emit(const clang::QualType &node, clang::ASTContext *ast,
                        int id) {
  if (auto *stream = ProtoStream::active()) {
    stream->node(node, ast, id);
    return true;
  }
  return false;
}
