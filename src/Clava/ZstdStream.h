#pragma once

#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <vector>

struct ZSTD_CCtx_s;

namespace clava {

/** Streams a Zstandard frame to another LLVM output stream. */
class ZstdStream final : public llvm::raw_ostream {
public:
  static llvm::Expected<std::unique_ptr<ZstdStream>> create(
      llvm::raw_ostream &Output, int CompressionLevel);

  ~ZstdStream() override;

  ZstdStream(const ZstdStream &) = delete;
  ZstdStream &operator=(const ZstdStream &) = delete;

  /** Flushes the final Zstandard frame bytes. Must be called exactly once. */
  llvm::Error finish();

private:
  ZstdStream(llvm::raw_ostream &Output, ZSTD_CCtx_s *Context,
             size_t BufferSize);

  void write_impl(const char *Data, size_t Size) override;
  uint64_t current_pos() const override;
  void recordError(size_t Result);

  llvm::raw_ostream &Output;
  ZSTD_CCtx_s *Context;
  std::vector<char> Buffer;
  uint64_t Position = 0;
  std::string ErrorMessage;
  bool Finished = false;
};

} // namespace clava
