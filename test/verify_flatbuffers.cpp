#include "complete_generated.h"
#include "FlatSchemaHash.h"

#include <flatbuffers/flatbuffers.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace fb = astwire::v2;

static uint32_t readSize(const uint8_t *data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: verify_flatbuffers <dump>\n";
    return 2;
  }

  std::ifstream input(argv[1], std::ios::binary);
  if (!input) {
    std::cerr << "cannot open FlatBuffers dump: " << argv[1] << "\n";
    return 2;
  }
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
  if (bytes.empty()) {
    std::cerr << "empty FlatBuffers dump\n";
    return 1;
  }

  uint64_t records = 0;
  uint64_t nodes = 0;
  uint64_t files = 0;
  const fb::End *end = nullptr;
  bool sawHeader = false;
  bool sawEnd = false;
  size_t offset = 0;
  size_t blocks = 0;
  while (offset < bytes.size()) {
    if (bytes.size() - offset < sizeof(uint32_t)) {
      std::cerr << "truncated size prefix at byte " << offset << "\n";
      return 1;
    }
    const uint32_t payloadSize = readSize(bytes.data() + offset);
    const size_t blockSize = sizeof(uint32_t) + payloadSize;
    if (payloadSize == 0 || blockSize > bytes.size() - offset) {
      std::cerr << "invalid size-prefixed block at byte " << offset << "\n";
      return 1;
    }

    flatbuffers::Verifier verifier(bytes.data() + offset, blockSize);
    if (!fb::VerifySizePrefixedBlockBuffer(verifier)) {
      std::cerr << "FlatBuffers verifier rejected block " << blocks << "\n";
      return 1;
    }
    const fb::Block *block = fb::GetSizePrefixedBlock(bytes.data() + offset);
    if (!block->records() || block->records()->size() == 0) {
      std::cerr << "empty record block " << blocks << "\n";
      return 1;
    }

    for (const fb::Record *record : *block->records()) {
      if (!record) {
        std::cerr << "null record in block " << blocks << "\n";
        return 1;
      }
      if (sawEnd) {
        std::cerr << "end record must be terminal\n";
        return 1;
      }
      ++records;
      switch (record->payload_type()) {
      case fb::RecordPayload::Header: {
        if (sawHeader || records != 1) {
          std::cerr << "header must be the first and only header record\n";
          return 1;
        }
        sawHeader = true;
        const fb::Header *header = record->payload_as_Header();
        if (!header->schema_hash() ||
            header->schema_hash()->string_view() != clava::flat::SchemaHash) {
          std::cerr << "schema hash mismatch in FlatBuffers header\n";
          return 1;
        }
        break;
      }
      case fb::RecordPayload::File: {
        const fb::File *file = record->payload_as_File();
        if (!file->id().has_value() || *file->id() == 0) {
          std::cerr << "file record is missing a nonzero id\n";
          return 1;
        }
        ++files;
        break;
      }
      case fb::RecordPayload::Node: {
        const fb::Node *node = record->payload_as_Node();
        if (!sawHeader || !node->id().has_value() || *node->id() <= 0) {
          std::cerr << "node record is missing a positive dense id or header\n";
          return 1;
        }
        ++nodes;
        break;
      }
      case fb::RecordPayload::End:
        if (end) {
          std::cerr << "multiple end records\n";
          return 1;
        }
        end = record->payload_as_End();
        if (!end->records().has_value() || !end->nodes().has_value() ||
            !end->files().has_value() || !end->ids().has_value()) {
          std::cerr << "end record is missing required counters\n";
          return 1;
        }
        sawEnd = true;
        break;
      default:
        if (!sawHeader) {
          std::cerr << "header must be the first record\n";
          return 1;
        }
        break;
      }
    }

    offset += blockSize;
    ++blocks;
  }

  if (!sawHeader || !end) {
    std::cerr << "FlatBuffers stream is missing its header or end record\n";
    return 1;
  }
  if (*end->records() + 1 != records || *end->nodes() != nodes ||
      *end->files() != files) {
    std::cerr << "end-record counts do not match the decoded stream\n";
    return 1;
  }
  std::cout << "blocks=" << blocks << " records=" << records
            << " nodes=" << nodes << " files=" << files << "\n";
  return 0;
}
