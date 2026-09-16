#include "clava_ast_wire.pb.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view Magic = "CLAVAPB1";
constexpr uint64_t MaxFrameBytes = 64 * 1024 * 1024;
constexpr uint32_t ProtocolMajor = 1;
constexpr uint32_t ProtocolMinor = 1;

void appendVarint(std::string &output, uint64_t value) {
  while (value >= 0x80) {
    output.push_back(static_cast<char>((value & 0x7f) | 0x80));
    value >>= 7;
  }
  output.push_back(static_cast<char>(value));
}

bool readVarint(std::string_view input, size_t &offset, uint64_t &value) {
  value = 0;
  for (unsigned shift = 0; shift < 64; shift += 7) {
    if (offset == input.size())
      return false;
    const auto byte = static_cast<unsigned char>(input[offset++]);
    if (shift == 63 && byte > 1)
      return false;
    value |= static_cast<uint64_t>(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0)
      return true;
  }
  return false;
}

bool validEnvelope(const astwire::v1::Envelope &envelope) {
  switch (envelope.payload_case()) {
  case astwire::v1::Envelope::kHeader: {
    const auto &header = envelope.header();
    return header.has_protocol_major() && header.has_protocol_minor() &&
           header.has_schema_id() && header.has_producer_version() &&
           header.has_llvm_major() && header.has_schema_sha256() &&
           header.protocol_major() == ProtocolMajor &&
           header.protocol_minor() == ProtocolMinor;
  }
  case astwire::v1::Envelope::kChunk:
    if (envelope.chunk().records_size() == 0)
      return false;
    for (const auto &record : envelope.chunk().records()) {
      if (record.record_case() == astwire::v1::Record::RECORD_NOT_SET)
        return false;
    }
    return true;
  case astwire::v1::Envelope::kEnd: {
    const auto &end = envelope.end();
    return end.has_records() && end.has_nodes() && end.has_raw_bytes() &&
           end.has_files() && end.has_ids();
  }
  case astwire::v1::Envelope::PAYLOAD_NOT_SET:
    return false;
  }
  return false;
}

bool readEnvelope(std::string_view input, size_t &offset,
                  astwire::v1::Envelope &envelope,
                  size_t *frame_start = nullptr) {
  if (frame_start != nullptr)
    *frame_start = offset;
  uint64_t length = 0;
  if (!readVarint(input, offset, length) || length == 0 ||
      length > MaxFrameBytes || length > input.size() - offset)
    return false;
  envelope.Clear();
  if (!envelope.ParseFromArray(input.data() + offset,
                               static_cast<int>(length)))
    return false;
  if (!validEnvelope(envelope))
    return false;
  offset += static_cast<size_t>(length);
  return true;
}

void appendEnvelope(std::string &stream,
                    const astwire::v1::Envelope &envelope) {
  std::string payload;
  assert(envelope.SerializeToString(&payload));
  appendVarint(stream, payload.size());
  stream += payload;
}

std::string makeValidStream() {
  std::string stream(Magic);

  astwire::v1::Envelope header;
  auto *header_body = header.mutable_header();
  header_body->set_protocol_major(ProtocolMajor);
  header_body->set_protocol_minor(ProtocolMinor);
  header_body->set_schema_id("clava-ast-wire");
  header_body->set_producer_version("test");
  header_body->set_llvm_major(18);
  header_body->set_schema_sha256("schema");
  appendEnvelope(stream, header);

  astwire::v1::Envelope first_chunk;
  auto *file = first_chunk.mutable_chunk()->add_records()->mutable_file();
  file->set_id(1);
  file->set_path("source.cpp");
  appendEnvelope(stream, first_chunk);

  astwire::v1::Envelope second_chunk;
  auto *include = second_chunk.mutable_chunk()->add_records()->mutable_include();
  include->set_source("source.cpp");
  include->set_name("header.h");
  include->set_line(1);
  include->set_angled(false);
  appendEnvelope(stream, second_chunk);

  astwire::v1::Envelope end;
  auto *end_body = end.mutable_end();
  end_body->set_records(2);
  end_body->set_nodes(0);
  end_body->set_raw_bytes(stream.size());
  end_body->set_files(1);
  end_body->set_ids(0);
  appendEnvelope(stream, end);
  return stream;
}

bool validStream(std::string_view stream) {
  if (!stream.starts_with(Magic))
    return false;

  size_t offset = Magic.size();
  bool header_seen = false;
  bool end_seen = false;
  uint64_t records = 0;
  uint64_t nodes = 0;
  uint64_t files = 0;
  astwire::v1::Envelope envelope;
  while (offset < stream.size()) {
    size_t frame_start = offset;
    if (!readEnvelope(stream, offset, envelope, &frame_start))
      return false;

    switch (envelope.payload_case()) {
    case astwire::v1::Envelope::kHeader:
      if (header_seen || frame_start != Magic.size())
        return false;
      header_seen = true;
      break;
    case astwire::v1::Envelope::kChunk:
      if (!header_seen || end_seen)
        return false;
      for (const auto &record : envelope.chunk().records()) {
        ++records;
        if (record.record_case() == astwire::v1::Record::kNode)
          ++nodes;
        if (record.record_case() == astwire::v1::Record::kFile)
          ++files;
      }
      break;
    case astwire::v1::Envelope::kEnd:
      if (!header_seen || end_seen || envelope.end().records() != records ||
          envelope.end().nodes() != nodes || envelope.end().files() != files ||
          envelope.end().raw_bytes() != frame_start)
        return false;
      end_seen = true;
      if (offset != stream.size())
        return false;
      break;
    case astwire::v1::Envelope::PAYLOAD_NOT_SET:
      return false;
    }
  }
  return header_seen && end_seen;
}

} // namespace

int main() {
  const std::string valid = makeValidStream();
  assert(validStream(valid));

  std::string badMagic = valid;
  badMagic[0] = 'X';
  assert(!validStream(badMagic));

  std::string truncated = valid.substr(0, valid.size() - 1);
  assert(!validStream(truncated));

  std::string truncatedLength(Magic);
  truncatedLength.push_back(static_cast<char>(0x80));
  assert(!validStream(truncatedLength));

  std::string oversized(Magic);
  appendVarint(oversized, MaxFrameBytes + 1);
  assert(!validStream(oversized));

  astwire::v1::Envelope incompatible;
  incompatible.mutable_header()->set_protocol_major(2);
  incompatible.mutable_header()->set_protocol_minor(ProtocolMinor);
  incompatible.mutable_header()->set_schema_id("clava-ast-wire");
  incompatible.mutable_header()->set_producer_version("test");
  incompatible.mutable_header()->set_llvm_major(18);
  incompatible.mutable_header()->set_schema_sha256("schema");
  std::string incompatible_stream(Magic);
  appendEnvelope(incompatible_stream, incompatible);
  assert(!validStream(incompatible_stream));

  astwire::v1::Envelope empty_chunk;
  empty_chunk.mutable_chunk();
  std::string empty_chunk_stream(Magic);
  astwire::v1::Envelope header;
  auto *header_body = header.mutable_header();
  header_body->set_protocol_major(ProtocolMajor);
  header_body->set_protocol_minor(ProtocolMinor);
  header_body->set_schema_id("clava-ast-wire");
  header_body->set_producer_version("test");
  header_body->set_llvm_major(18);
  header_body->set_schema_sha256("schema");
  appendEnvelope(empty_chunk_stream, header);
  appendEnvelope(empty_chunk_stream, empty_chunk);
  assert(!validStream(empty_chunk_stream));

  astwire::v1::Envelope missing_header;
  missing_header.mutable_header()->set_protocol_major(ProtocolMajor);
  std::string missing_header_stream(Magic);
  appendEnvelope(missing_header_stream, missing_header);
  assert(!validStream(missing_header_stream));

  astwire::v1::Envelope bad_end;
  bad_end.mutable_end()->set_records(0);
  std::string bad_end_stream(Magic);
  appendEnvelope(bad_end_stream, bad_end);
  assert(!validStream(bad_end_stream));
  return 0;
}
