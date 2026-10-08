#include "ZstdStream.h"

#include "llvm/Support/Errc.h"

#include <zstd.h>

namespace clava {

llvm::Expected<std::unique_ptr<ZstdStream>> ZstdStream::create(
    llvm::raw_ostream &Output, int CompressionLevel) {
  ZSTD_CCtx *Context = ZSTD_createCCtx();
  if (Context == nullptr) {
    return llvm::createStringError(llvm::errc::not_enough_memory,
                                   "could not create Zstandard context");
  }

  const size_t Result = ZSTD_CCtx_setParameter(
      Context, ZSTD_c_compressionLevel, CompressionLevel);
  if (ZSTD_isError(Result)) {
    const std::string Message = ZSTD_getErrorName(Result);
    ZSTD_freeCCtx(Context);
    return llvm::createStringError(llvm::errc::invalid_argument,
                                   "could not configure Zstandard: %s",
                                   Message.c_str());
  }

  return std::unique_ptr<ZstdStream>(
      new ZstdStream(Output, Context, ZSTD_CStreamOutSize()));
}

ZstdStream::ZstdStream(llvm::raw_ostream &Output, ZSTD_CCtx *Context,
                       size_t BufferSize)
    : Output(Output), Context(Context), Buffer(BufferSize) {
  SetBufferSize(64 * 1024);
}

ZstdStream::~ZstdStream() {
  ZSTD_freeCCtx(Context);
}

void ZstdStream::recordError(size_t Result) {
  if (ErrorMessage.empty() && ZSTD_isError(Result)) {
    ErrorMessage = ZSTD_getErrorName(Result);
  }
}

void ZstdStream::write_impl(const char *Data, size_t Size) {
  Position += Size;
  if (!ErrorMessage.empty()) {
    return;
  }

  ZSTD_inBuffer Input{Data, Size, 0};
  while (Input.pos < Input.size) {
    ZSTD_outBuffer Compressed{Buffer.data(), Buffer.size(), 0};
    const size_t Result = ZSTD_compressStream2(
        Context, &Compressed, &Input, ZSTD_e_continue);
    recordError(Result);
    if (!ErrorMessage.empty()) {
      return;
    }

    Output.write(Buffer.data(), Compressed.pos);
  }
}

uint64_t ZstdStream::current_pos() const {
  return Position;
}

llvm::Error ZstdStream::finish() {
  if (Finished) {
    return llvm::createStringError(llvm::errc::invalid_argument,
                                   "Zstandard stream was already finished");
  }

  flush();
  Finished = true;
  if (!ErrorMessage.empty()) {
    return llvm::createStringError(llvm::errc::io_error, "%s",
                                   ErrorMessage.c_str());
  }

  ZSTD_inBuffer Input{nullptr, 0, 0};
  size_t Remaining = 1;
  while (Remaining != 0) {
    ZSTD_outBuffer Compressed{Buffer.data(), Buffer.size(), 0};
    Remaining = ZSTD_compressStream2(Context, &Compressed, &Input, ZSTD_e_end);
    recordError(Remaining);
    if (!ErrorMessage.empty()) {
      return llvm::createStringError(llvm::errc::io_error, "%s",
                                     ErrorMessage.c_str());
    }

    Output.write(Buffer.data(), Compressed.pos);
  }

  Output.flush();
  return llvm::Error::success();
}

} // namespace clava
