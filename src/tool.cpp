#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include "Clang/ClangAst.h"
#include "Clava/DumpStream.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"

#include <memory>
#include <system_error>


// Globally defined, or parser might not catch it
static llvm::cl::OptionCategory MyToolCategory("my-tool options");
static llvm::cl::opt<int> UserIdOption("id", llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<int> UserSystemHeaderThresholdOption(
        "system-header-threshold", llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<std::string> AstDumpOutputOption(
        "ast-dump-output", llvm::cl::value_desc("path"),
        llvm::cl::desc("Write the structured AST dump to path"),
        llvm::cl::cat(MyToolCategory));


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


  auto OptionsParser =
      clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory);

  if (auto E = OptionsParser.takeError()) {
    llvm::errs() << "Problem with division " << llvm::toString(std::move(E))
                 << "\n";
    return 1;
  }

  std::unique_ptr<llvm::raw_fd_ostream> dumpOutput;
  if (!AstDumpOutputOption.getValue().empty()) {
    std::error_code ErrorCode;
    dumpOutput = std::make_unique<llvm::raw_fd_ostream>(
        AstDumpOutputOption, ErrorCode, llvm::sys::fs::OF_None);
    if (ErrorCode) {
      llvm::errs() << "Cannot open AST dump output '" << AstDumpOutputOption
                   << "': " << ErrorCode.message() << "\n";
      return 1;
    }

    clava::setDumpStream(*dumpOutput);
  }

  clang::tooling::ClangTool Tool((*OptionsParser).getCompilations(),
                                 (*OptionsParser).getSourcePathList());

  DumpResources::init(UserIdOption.getValue(),
                      UserSystemHeaderThresholdOption.getValue());

  int returnValue =
      Tool.run(clang::tooling::newFrontendActionFactory<DumpAstAction>().get());

  DumpResources::finish();

  if (dumpOutput) {
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
