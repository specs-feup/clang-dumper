#ifndef CLANG_DUMPER_DUMP_STREAM_H
#define CLANG_DUMPER_DUMP_STREAM_H

#include "llvm/Support/raw_ostream.h"

namespace clava {

/**
 * Returns the stream used for the structured AST dump protocol.
 *
 * The default is llvm::errs() to preserve the historical standalone-tool and
 * plugin behaviour. Callers that need diagnostics and protocol data on
 * separate streams can install their own stream for the duration of a run.
 */
llvm::raw_ostream &dumpStream();

/**
 * Installs the stream used for structured AST dump output.
 *
 * The caller owns the stream and must keep it alive until all dumping has
 * completed. This function is process-global because clang-dumper's existing
 * dumping interface is process-global.
 */
void setDumpStream(llvm::raw_ostream &stream);

} // namespace clava

#endif
