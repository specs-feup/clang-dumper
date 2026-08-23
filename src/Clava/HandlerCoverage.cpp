//
// Created by the clang-dumper refactor (2026).
//

#include "HandlerCoverage.h"

#include "llvm/Support/raw_ostream.h"

#include <map>
#include <set>

namespace clava {

namespace {
bool enabled = false;
std::map<std::string, std::set<std::string>> fallbacks;
} // namespace

void enableHandlerCoverageReport() { enabled = true; }

void recordHandlerFallback(const char *family, const std::string &classname) {
    if (!enabled) {
        return;
    }
    fallbacks[family].insert(classname);
}

void reportHandlerCoverage() {
    if (!enabled) {
        return;
    }

    if (fallbacks.empty()) {
        llvm::outs() << "[clang-dumper] handler coverage: all encountered "
                        "classes have dedicated handlers\n";
        return;
    }

    llvm::outs() << "[clang-dumper] handler coverage: classes using default "
                    "(fallback) handling:\n";
    for (const auto &entry : fallbacks) {
        llvm::outs() << "  " << entry.first << ":\n";
        for (const auto &classname : entry.second) {
            llvm::outs() << "    " << classname << "\n";
        }
    }
}

} // namespace clava
