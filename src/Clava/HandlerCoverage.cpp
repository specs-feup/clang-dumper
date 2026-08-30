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
bool encountersEnabled = false;
std::map<std::string, std::set<std::string>> fallbacks;
std::map<std::string, std::set<std::string>> encounters;
} // namespace

void enableHandlerCoverageReport() { enabled = true; }

void enableHandlerEncounterReport() { encountersEnabled = true; }

void recordHandlerEncounter(const char *family, const std::string &classname) {
    if (!encountersEnabled) {
        return;
    }
    encounters[family].insert(classname);
}

void recordHandlerFallback(const char *family, const std::string &classname) {
    if (!enabled) {
        return;
    }
    fallbacks[family].insert(classname);
}

void reportHandlerCoverage() {
    if (encountersEnabled) {
        llvm::outs() << "[clang-dumper] handler encounters:\n";
        if (encounters.empty()) {
            llvm::outs() << "  (none)\n";
        }
        for (const auto &entry : encounters) {
            llvm::outs() << "  " << entry.first << ":";
            for (const auto &classname : entry.second) {
                llvm::outs() << " " << classname;
            }
            llvm::outs() << "\n";
        }
    }

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
