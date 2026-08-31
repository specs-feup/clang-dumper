#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Basic/MakeSupport.h>

#include "Clang/ClangAst.h"
#include "Clava/DumpStream.h"
#include "Clava/ZstdStream.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"

#include <memory>
#include <system_error>
#include <vector>


// Globally defined, or parser might not catch it
static llvm::cl::OptionCategory MyToolCategory("my-tool options");
static llvm::cl::opt<int> UserIdOption("id", llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<int> UserSystemHeaderThresholdOption(
        "system-header-threshold", llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<bool> CompileOnlyOption(
        "c", llvm::cl::desc("Parse without linking"),
        llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<std::string> AstDumpOutputOption(
        "o", llvm::cl::value_desc("path"),
        llvm::cl::desc("Write the structured AST dump to path"),
        llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<bool> DependencyOption(
        "MD", llvm::cl::desc("Write a Make dependency file including system headers"),
        llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<std::string> DependencyFileOption(
        "MF", llvm::cl::value_desc("path"),
        llvm::cl::desc("Write dependencies to path"),
        llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<std::string> AstDumpCompressionOption(
        "ast-dump-compression", llvm::cl::value_desc("none|zstd"),
        llvm::cl::desc("Compress the structured AST output"),
        llvm::cl::init("none"), llvm::cl::cat(MyToolCategory));


/**
 * ccache canonicalizes a compiler invocation to put compiler flags before
 * `--` and the source after it. CommonOptionsParser uses the inverse layout:
 * tool flags and sources before `--`, compiler flags after it. Translate only
 * that unambiguous ccache shape and leave ordinary tool invocations untouched.
 */
static std::vector<std::string> normalizeCcacheArguments(
        int argc, const char *argv[]) {
  if (argc < 3 || std::string(argv[argc - 2]) != "--" ||
      llvm::StringRef(argv[argc - 1]).starts_with("-")) {
    return {};
  }

  std::vector<std::string> ToolArguments{argv[0]};
  std::vector<std::string> CompilerArguments;

  for (int Index = 1; Index < argc - 2; ++Index) {
    const std::string Argument = argv[Index];
    const bool HasSeparateValue =
        Argument == "-o" || Argument == "-MF" || Argument == "-id" ||
        Argument == "-system-header-threshold" ||
        Argument == "-ast-dump-compression";
    const bool IsToolArgument =
        Argument == "-c" || Argument == "-MD" || HasSeparateValue ||
        llvm::StringRef(Argument).starts_with("-o=") ||
        llvm::StringRef(Argument).starts_with("-MF=") ||
        llvm::StringRef(Argument).starts_with("-id=") ||
        llvm::StringRef(Argument).starts_with("-system-header-threshold=") ||
        llvm::StringRef(Argument).starts_with("-ast-dump-compression=");

    if (!IsToolArgument) {
      CompilerArguments.push_back(Argument);
      continue;
    }

    ToolArguments.push_back(Argument);
    if (HasSeparateValue) {
      if (++Index >= argc - 2) {
        return {};
      }
      ToolArguments.push_back(argv[Index]);
    }
  }

  ToolArguments.push_back(argv[argc - 1]);
  ToolArguments.push_back("--");
  ToolArguments.insert(ToolArguments.end(), CompilerArguments.begin(),
                       CompilerArguments.end());
  return ToolArguments;
}


int main(int argc, const char *argv[]) {

  llvm::InitLLVM X(argc, argv);

  // Register the native target's MC subsystem and asm parser. The build only
  // links the host-arch backend archives, and nothing else triggers their
  // registration: without this, plain parsing works but anything needing the
  // MC layer (e.g. MS-style inline assembly) fails with
  // "(no targets are registered)".
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // Errs is the main way we dump information, we tested if making it buffered
  // improved performance but could not detect a significant difference
  // llvm::errs().SetBuffered();


  auto NormalizedArguments = normalizeCcacheArguments(argc, argv);
  std::vector<const char *> NormalizedArgv;
  if (!NormalizedArguments.empty()) {
    for (const auto &Argument : NormalizedArguments) {
      NormalizedArgv.push_back(Argument.c_str());
    }
    argc = static_cast<int>(NormalizedArgv.size());
    argv = NormalizedArgv.data();
  }

  auto OptionsParser =
      clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory);

  if (auto E = OptionsParser.takeError()) {
    llvm::errs() << "Problem with division " << llvm::toString(std::move(E))
                 << "\n";
    return 1;
  }

  if (DependencyOption && DependencyFileOption.empty()) {
    llvm::errs() << "-MD requires -MF <path>\n";
    return 1;
  }

  if (!DependencyOption && !DependencyFileOption.empty()) {
    llvm::errs() << "-MF requires -MD\n";
    return 1;
  }

  if (AstDumpCompressionOption != "none" &&
      AstDumpCompressionOption != "zstd") {
    llvm::errs() << "Unsupported AST dump compression '"
                 << AstDumpCompressionOption << "'\n";
    return 1;
  }

  if (AstDumpCompressionOption == "zstd" && AstDumpOutputOption.empty()) {
    llvm::errs() << "-ast-dump-compression=zstd requires -o <path>\n";
    return 1;
  }

  std::unique_ptr<llvm::raw_fd_ostream> dumpOutput;
  std::unique_ptr<clava::ZstdStream> compressedDumpOutput;
  if (!AstDumpOutputOption.getValue().empty()) {
    std::error_code ErrorCode;
    dumpOutput = std::make_unique<llvm::raw_fd_ostream>(
        AstDumpOutputOption, ErrorCode, llvm::sys::fs::OF_None);
    if (ErrorCode) {
      llvm::errs() << "Cannot open AST dump output '" << AstDumpOutputOption
                   << "': " << ErrorCode.message() << "\n";
      return 1;
    }

    if (AstDumpCompressionOption == "zstd") {
      // Fast level 5 keeps producer overhead close to plain output while still
      // reducing large text dumps by roughly an order of magnitude.
      auto CompressedOutput = clava::ZstdStream::create(*dumpOutput, -5);
      if (!CompressedOutput) {
        llvm::errs() << "Cannot initialize compressed AST dump output: "
                     << llvm::toString(CompressedOutput.takeError()) << "\n";
        return 1;
      }
      compressedDumpOutput = std::move(*CompressedOutput);
      clava::setDumpStream(*compressedDumpOutput);
    } else {
      clava::setDumpStream(*dumpOutput);
    }
  }

  clang::tooling::ClangTool Tool((*OptionsParser).getCompilations(),
                                 (*OptionsParser).getSourcePathList());

  if (DependencyOption) {
    const std::string DependencyTarget = AstDumpOutputOption.empty()
        ? (*OptionsParser).getSourcePathList().front()
        : AstDumpOutputOption.getValue();
    llvm::SmallVector<char> QuotedDependencyTarget;
    clang::quoteMakeTarget(DependencyTarget, QuotedDependencyTarget);
    Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
        {"-Xclang", "-dependency-file", "-Xclang", DependencyFileOption,
         "-Xclang", "-MT", "-Xclang",
         std::string(QuotedDependencyTarget.begin(), QuotedDependencyTarget.end()),
         "-Xclang", "-sys-header-deps"},
        clang::tooling::ArgumentInsertPosition::END));
  }

  DumpResources::init(UserIdOption.getValue(),
                      UserSystemHeaderThresholdOption.getValue());

  int returnValue =
      Tool.run(clang::tooling::newFrontendActionFactory<DumpAstAction>().get());

  DumpResources::finish();

  if (dumpOutput) {
    if (compressedDumpOutput) {
      if (auto Error = compressedDumpOutput->finish()) {
        llvm::errs() << "Cannot compress AST dump output '"
                     << AstDumpOutputOption << "': "
                     << llvm::toString(std::move(Error)) << "\n";
        returnValue = 1;
      }
    }

    dumpOutput->flush();
    if (dumpOutput->has_error()) {
      llvm::errs() << "Cannot write AST dump output '" << AstDumpOutputOption
                   << "': " << dumpOutput->error().message() << "\n";
      dumpOutput->clear_error();
      returnValue = 1;
    }

    dumpOutput->close();
    if (dumpOutput->has_error()) {
      llvm::errs() << "Cannot close AST dump output '" << AstDumpOutputOption
                   << "': " << dumpOutput->error().message() << "\n";
      dumpOutput->clear_error();
      returnValue = 1;
    }
  }

  return returnValue;
}
