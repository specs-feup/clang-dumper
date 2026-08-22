//
// Created by the clang-dumper refactor (2026).
//

#ifndef CLANGASTDUMPER_HANDLERCOVERAGE_H
#define CLANGASTDUMPER_HANDLERCOVERAGE_H

#include <string>

namespace clava {

/**
 * Handler coverage self-check support.
 *
 * Dispatch tables map Clang class names to dedicated handlers; classes not
 * present in a table silently use default (fallback) handling. After an LLVM
 * upgrade, new or renamed classes show up as unexpected fallbacks. Running
 * the tool with -handler-coverage-report lists every encountered class name
 * that relied on fallback handling, so renames/additions are easy to spot
 * and fix by adding a single table entry.
 */
void enableHandlerCoverageReport();

/// Records that `classname` used fallback handling in dispatch `family`.
/// No-op unless coverage reporting is enabled.
void recordHandlerFallback(const char *family, const std::string &classname);

/// Prints the collected report to stdout. No-op unless enabled.
void reportHandlerCoverage();

} // namespace clava

#endif // CLANGASTDUMPER_HANDLERCOVERAGE_H
