#include "ResourceDir.h"

#include <clang/Basic/Version.h>
#include <clang/Driver/Driver.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/raw_ostream.h>

#include <optional>
#include <string>

namespace clava {
namespace {

// Installed Clang binaries to look for on PATH, most specific first
constexpr const char *CLANG_NAMES[] = {
    "clang-" CLANG_VERSION_MAJOR_STRING,
    "clang++-" CLANG_VERSION_MAJOR_STRING,
    "clang",
    "clang++",
};

// Clang binaries of installations that are not on PATH by default
constexpr const char *CLANG_PATHS[] = {
    // Debian/Ubuntu
    "/usr/lib/llvm-" CLANG_VERSION_MAJOR_STRING "/bin/clang",
    // Homebrew on Apple silicon and on Intel
    "/opt/homebrew/opt/llvm@" CLANG_VERSION_MAJOR_STRING "/bin/clang",
    "/usr/local/opt/llvm@" CLANG_VERSION_MAJOR_STRING "/bin/clang",
    // Windows installer
    "C:/Program Files/LLVM/bin/clang",
};

// The resource directory of the Clang binary at 'clangPath', if it has Clang's
// headers. Uses Clang's own rule, which puts this dumper's major version in
// the path, so a Clang of another major version never matches.
std::optional<std::string> resourceDirOf(llvm::StringRef clangPath) {
  auto dir = clang::driver::Driver::GetResourcesPath(clangPath);
  llvm::SmallString<256> header(dir);
  llvm::sys::path::append(header, "include", "stddef.h");
  if (!llvm::sys::fs::exists(header)) {
    return std::nullopt;
  }
  return dir;
}

// Clang derives its resource directory from the real path of its binary, not
// from a symlink such as /usr/bin/clang-18
std::optional<std::string> resourceDirOfProgram(llvm::StringRef name) {
  auto found = llvm::sys::findProgramByName(name);
  llvm::SmallString<256> real;
  if (!found || llvm::sys::fs::real_path(*found, real)) {
    return std::nullopt;
  }
  return resourceDirOf(real);
}

std::optional<std::string> findFallbackResourceDir() {
  static int anchor;
  auto self = llvm::sys::fs::getMainExecutable("clang-dumper", &anchor);
  if (resourceDirOf(self)) {
    return std::nullopt;
  }

  for (const char *name : CLANG_NAMES) {
    if (auto dir = resourceDirOfProgram(name)) {
      return dir;
    }
  }

  for (const char *path : CLANG_PATHS) {
    if (auto dir = resourceDirOf(path)) {
      return dir;
    }
  }

  llvm::errs() << "clang-dumper: warning: Clang resource directory '"
               << clang::driver::Driver::GetResourcesPath(self)
               << "' does not exist and no Clang " CLANG_VERSION_MAJOR_STRING
                  " installation was found; install one or pass"
                  " '-resource-dir=<dir>'\n";
  return std::nullopt;
}

bool needsFallback(const clang::tooling::CommandLineArguments &args) {
  return llvm::none_of(args, [](llvm::StringRef arg) {
    return arg.starts_with("-resource-dir") || arg == "-nostdinc" ||
           arg == "-nobuiltininc";
  });
}

} // namespace

clang::tooling::ArgumentsAdjuster getResourceDirFallbackAdjuster() {
  return [](const clang::tooling::CommandLineArguments &args,
            llvm::StringRef) {
    if (!needsFallback(args)) {
      return args;
    }

    // Searched once per process, and only if a compile needs it
    static const auto fallback = findFallbackResourceDir();
    if (!fallback) {
      return args;
    }

    // Inserts before a '--' end-of-options delimiter, as ClangTool does for
    // its own '-resource-dir'
    auto option = "-resource-dir=" + *fallback;
    return clang::tooling::getInsertArgumentAdjuster(option.c_str())(args, "");
  };
}

} // namespace clava
