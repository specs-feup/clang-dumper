#ifndef CLANGASTDUMPER_CLANGVERSION_H
#define CLANGASTDUMPER_CLANGVERSION_H

// Central place for LLVM-version-dependent code shims.
//
// When a Clang/LLVM bump changes an API, add a shim here guarded by
// LLVM_VERSION_MAJOR instead of scattering #if blocks across visitors:
//
//   #include "llvm/Config/llvm-config.h"
//   #if LLVM_VERSION_MAJOR >= 21
//   ... new API ...
//   #else
//   ... old API ...
//   #endif
//
// See docs/updating-llvm.md for the full version-bump procedure.

#include "llvm/Config/llvm-config.h"

#endif
