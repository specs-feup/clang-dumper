#include "DumpStream.h"

namespace {

llvm::raw_ostream *&configuredDumpStream() {
    static llvm::raw_ostream *stream = &llvm::errs();
    return stream;
}

} // namespace

llvm::raw_ostream &clava::dumpStream() { return *configuredDumpStream(); }

void clava::setDumpStream(llvm::raw_ostream &stream) {
    configuredDumpStream() = &stream;
}
