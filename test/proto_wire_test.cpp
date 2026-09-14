#include "clava_ast_wire.pb.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view Magic = "CLAVAPB1";
constexpr uint64_t MaxRecordBytes = 64 * 1024 * 1024;

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

bool readEnvelope(std::string_view input, size_t &offset,
                  astwire::v1::Envelope &envelope) {
  uint64_t length = 0;
  if (!readVarint(input, offset, length) || length == 0 ||
      length > MaxRecordBytes || length > input.size() - offset)
    return false;
  if (!envelope.ParseFromArray(input.data() + offset,
                               static_cast<int>(length)))
    return false;
  if (envelope.has_header()) {
    const auto &header = envelope.header();
    if (!header.has_protocol_major() || !header.has_protocol_minor() ||
        !header.has_schema_id() || !header.has_producer_version() ||
        !header.has_llvm_major() || !header.has_schema_sha256() ||
        header.protocol_major() != 1)
      return false;
  } else if (envelope.has_record()) {
    if (envelope.record().record_case() == astwire::v1::Record::RECORD_NOT_SET)
      return false;
  } else if (!envelope.has_end()) {
    return false;
  }
  offset += static_cast<size_t>(length);
  return true;
}

std::string makeValidStream() {
  std::string stream(Magic);
  astwire::v1::Envelope header;
  header.mutable_header()->set_protocol_major(1);
  header.mutable_header()->set_protocol_minor(0);
  header.mutable_header()->set_schema_id("clava-ast-wire");
  header.mutable_header()->set_producer_version("test");
  header.mutable_header()->set_llvm_major(18);
  header.mutable_header()->set_schema_sha256("schema");
  std::string payload;
  assert(header.SerializeToString(&payload));
  appendVarint(stream, payload.size());
  stream += payload;

  astwire::v1::Envelope end;
  end.mutable_end()->set_records(2);
  assert(end.SerializeToString(&payload));
  appendVarint(stream, payload.size());
  stream += payload;
  return stream;
}

} // namespace

int main() {
  const std::string valid = makeValidStream();
  std::string payload;
  assert(valid.starts_with(Magic));
  size_t offset = Magic.size();
  astwire::v1::Envelope envelope;
  assert(readEnvelope(valid, offset, envelope));
  assert(envelope.has_header());
  assert(envelope.header().protocol_major() == 1);
  assert(readEnvelope(valid, offset, envelope));
  assert(envelope.has_end());
  assert(offset == valid.size());

  std::string badMagic = valid;
  badMagic[0] = 'X';
  assert(!std::string_view(badMagic).starts_with(Magic));

  std::string truncated = valid.substr(0, valid.size() - 1);
  offset = Magic.size();
  assert(readEnvelope(truncated, offset, envelope));
  assert(!readEnvelope(truncated, offset, envelope));

  std::string truncatedLength(Magic);
  truncatedLength.push_back(static_cast<char>(0x80));
  offset = Magic.size();
  assert(!readEnvelope(truncatedLength, offset, envelope));

  std::string oversized(Magic);
  appendVarint(oversized, MaxRecordBytes + 1);
  offset = Magic.size();
  assert(!readEnvelope(oversized, offset, envelope));

  astwire::v1::Envelope incompatible;
  incompatible.mutable_header()->set_protocol_major(2);
  incompatible.mutable_header()->set_protocol_minor(0);
  incompatible.mutable_header()->set_schema_id("clava-ast-wire");
  incompatible.mutable_header()->set_producer_version("test");
  incompatible.mutable_header()->set_llvm_major(18);
  incompatible.mutable_header()->set_schema_sha256("schema");
  std::string incompatibleStream(Magic);
  assert(incompatible.SerializeToString(&payload));
  appendVarint(incompatibleStream, payload.size());
  incompatibleStream += payload;
  offset = Magic.size();
  assert(!readEnvelope(incompatibleStream, offset, envelope));

  astwire::v1::Envelope missingHeaderField;
  missingHeaderField.mutable_header()->set_protocol_major(1);
  std::string missingHeaderStream(Magic);
  assert(missingHeaderField.SerializeToString(&payload));
  appendVarint(missingHeaderStream, payload.size());
  missingHeaderStream += payload;
  offset = Magic.size();
  assert(!readEnvelope(missingHeaderStream, offset, envelope));
  return 0;
}
