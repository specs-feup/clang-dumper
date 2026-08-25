#ifndef CLANG_DUMPER_RESOURCE_DIR_H
#define CLANG_DUMPER_RESOURCE_DIR_H

#include <string>

namespace clava {

/**
 * Returns a system Clang resource directory to use when the default one,
 * resolved relative to the executable path, is missing or incomplete.
 *
 * Returns an empty string when the default resource directory is usable, or
 * when no system resource directory for this binary's LLVM major version
 * could be found (in which case a diagnostic is printed to stderr).
 */
std::string findFallbackResourceDir(const std::string &executablePath);

}

#endif
