#ifndef CLANGASTDUMPER_RESOURCEDIR_H
#define CLANGASTDUMPER_RESOURCEDIR_H

#include <clang/Tooling/ArgumentsAdjusters.h>

namespace clava {

/// Clang finds its own headers (stddef.h, stdarg.h, ...) in a resource
/// directory derived from the executable's path, which a relocated dumper
/// binary does not have. This adjuster points such compiles at the resource
/// directory of an installed Clang with the same major version.
///
/// Compiles that pass '-resource-dir', or that do not use Clang's headers
/// ('-nostdinc', '-nobuiltininc'), are left unchanged.
clang::tooling::ArgumentsAdjuster getResourceDirFallbackAdjuster();

} // namespace clava

#endif
