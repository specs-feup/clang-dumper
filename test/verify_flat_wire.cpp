#include "wire_generated.h"

#include "flatbuffers/base.h"
#include "flatbuffers/verifier.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace {

std::vector<uint8_t> readFile(const char *path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("cannot open input");
  }
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
}

void fail(const char *message) { throw std::runtime_error(message); }

void verify(const std::vector<uint8_t> &bytes) {
  size_t offset = 0;
  uint64_t records = 0;
  uint64_t nodes = 0;
  uint64_t rawBytes = 0;
  uint32_t files = 0;
  bool header = false;
  bool ended = false;

  while (offset < bytes.size()) {
    if (bytes.size() - offset < sizeof(uint32_t)) {
      fail("truncated size prefix");
    }
    const auto length = flatbuffers::ReadScalar<uint32_t>(bytes.data() + offset);
    if (length == 0 || length > 16 * 1024 * 1024 ||
        length + sizeof(uint32_t) > bytes.size() - offset) {
      fail("invalid record size");
    }

    flatbuffers::Verifier verifier(bytes.data() + offset,
                                   length + sizeof(uint32_t));
    if (!astwire::flat::VerifySizePrefixedRecordBuffer(verifier)) {
      fail("FlatBuffer verifier rejected record");
    }
    const auto *record = astwire::flat::GetSizePrefixedRecord(
        bytes.data() + offset);
    if (ended || record->body_type() == astwire::flat::RecordBody::NONE) {
      fail("record after end or missing body");
    }
    ++records;

    switch (record->body_type()) {
    case astwire::flat::RecordBody::Header:
      if (header || records != 1 ||
          record->body_as_Header()->schema_id()->str() != "ast-wire-flat-v1") {
        fail("invalid header");
      }
      header = true;
      break;
    case astwire::flat::RecordBody::Raw: {
      if (!header || record->body_as_Raw()->bytes()->size() > 64 * 1024) {
        fail("invalid raw record");
      }
      rawBytes += record->body_as_Raw()->bytes()->size();
      break;
    }
    case astwire::flat::RecordBody::File:
      if (!header || record->body_as_File()->id() != ++files) {
        fail("non-contiguous file record");
      }
      break;
    case astwire::flat::RecordBody::Node:
      if (!header) {
        fail("node before header");
      }
      ++nodes;
      break;
    case astwire::flat::RecordBody::End: {
      const auto *end = record->body_as_End();
      if (!header || end->records() != records || end->nodes() != nodes ||
          end->raw_bytes() != rawBytes || end->files() != files) {
        fail("invalid end counters");
      }
      ended = true;
      break;
    }
    default:
      fail("unknown record body");
    }
    offset += length + sizeof(uint32_t);
  }

  if (!header || !ended) {
    fail("missing header or end");
  }
  std::cout << "records=" << records << " nodes=" << nodes
            << " raw_bytes=" << rawBytes << " files=" << files << '\n';
}

} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 2) {
      return 2;
    }
    verify(readFile(argv[1]));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
