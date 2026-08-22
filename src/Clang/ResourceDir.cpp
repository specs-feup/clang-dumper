#include "ResourceDir.h"

#include "clang/Driver/Driver.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Program.h"

#include <fstream>
#include <optional>
#include <string>
#include <vector>

using namespace llvm;

namespace clava {

namespace {

const std::string LLVM_MAJOR = std::to_string(LLVM_VERSION_MAJOR);

constexpr unsigned QUERY_TIMEOUT_SECONDS = 10;

bool hasInternalHeaders(StringRef resourceDir) {
  SmallString<256> headerPath(resourceDir);
  sys::path::append(headerPath, "include", "stddef.h");
  return sys::fs::exists(headerPath);
}

bool isValidResourceDir(StringRef resourceDir) {
  return sys::path::filename(resourceDir) == LLVM_MAJOR
      && hasInternalHeaders(resourceDir);
}

std::optional<std::string> queryPrintResourceDir(StringRef program) {
  int fd = -1;
  SmallString<128> outputPath;
  if (auto ec = sys::fs::createTemporaryFile("clang-dumper-resdir", "txt", fd,
                                             outputPath)) {
    return std::nullopt;
  }

  sys::Process::SafelyCloseFileDescriptor(fd);

  std::optional<StringRef> redirects[] = {std::nullopt,
                                          std::optional<StringRef>(outputPath.str()),
                                          std::optional<StringRef>(StringRef(""))};

  StringRef args[] = {program, "-print-resource-dir"};
  int exitCode = sys::ExecuteAndWait(program, args, std::nullopt, redirects,
                                     QUERY_TIMEOUT_SECONDS);

  std::string result;
  if (exitCode == 0) {
    std::ifstream output(outputPath.c_str());
    if (output) {
      std::getline(output, result);
      while (!result.empty()
             && (result.back() == '\r' || result.back() == ' ')) {
        result.pop_back();
      }
    }
  }

  sys::fs::remove(outputPath);

  if (!isValidResourceDir(result)) {
    return std::nullopt;
  }

  return result;
}

std::optional<std::string> querySystemClang(const std::string &selfPath) {
  const std::vector<std::string> commandNames = {"clang++-" + LLVM_MAJOR,
                                                 "clang-" + LLVM_MAJOR,
                                                 "clang++", "clang"};

  for (const auto &commandName : commandNames) {
    auto program = sys::findProgramByName(commandName);
    if (!program || program->empty()) {
      continue;
    }

    SmallString<256> selfCanonical(selfPath);
    SmallString<256> programCanonical(*program);
    sys::fs::make_absolute(selfCanonical);
    sys::fs::make_absolute(programCanonical);
    if (sys::fs::equivalent(selfCanonical, programCanonical)) {
      continue;
    }

    if (auto resourceDir = queryPrintResourceDir(*program)) {
      return resourceDir;
    }
  }

  return std::nullopt;
}

std::optional<std::string> scanKnownLocations() {
  const std::vector<std::string> candidates = {
      "/usr/lib/llvm-" + LLVM_MAJOR + "/lib/clang/" + LLVM_MAJOR,
      "/usr/lib/clang/" + LLVM_MAJOR,
      "/usr/local/lib/clang/" + LLVM_MAJOR,
      "/opt/homebrew/opt/llvm@" + LLVM_MAJOR + "/lib/clang/" + LLVM_MAJOR,
      "/opt/homebrew/opt/llvm/lib/clang/" + LLVM_MAJOR,
      "/Library/Developer/CommandLineTools/usr/lib/clang/" + LLVM_MAJOR,
      "C:/Program Files/LLVM/lib/clang/" + LLVM_MAJOR};

  for (const auto &candidate : candidates) {
    if (hasInternalHeaders(candidate)) {
      return candidate;
    }
  }

  return std::nullopt;
}

} // namespace

std::string findFallbackResourceDir(const std::string &executablePath) {
  std::string defaultDir =
      clang::driver::Driver::GetResourcesPath(executablePath);

  if (defaultDir.empty() || hasInternalHeaders(defaultDir)) {
    return "";
  }

  outs() << "[clang-dumper] Default Clang resource directory '" << defaultDir
         << "' is missing or incomplete\n";

  std::optional<std::string> found = querySystemClang(executablePath);
  if (!found) {
    found = scanKnownLocations();
  }

  if (found) {
    outs() << "[clang-dumper] Falling back to system Clang resource directory '"
           << *found << "'\n";
    outs().flush();
    return *found;
  }

  outs().flush();
  errs() << "[clang-dumper] Could not locate a Clang resource directory for "
            "LLVM "
         << LLVM_MAJOR << " on this system.\n"
         << "[clang-dumper] Install a matching system Clang to use the host's "
            "headers in SYSTEM mode (e.g. 'apt install clang-"
         << LLVM_MAJOR << "' or 'brew install llvm@" << LLVM_MAJOR
         << "').\n[clang-dumper] Without it, parsing fails with errors such as "
            "'stddef.h' file not found'.\n";

  return "";
}

}
