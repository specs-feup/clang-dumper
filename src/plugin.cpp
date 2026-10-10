#include <clang/Frontend/FrontendPluginRegistry.h>

#include "Clang/ClangAst.h"
#include "Clang/ClangNodes.h"
#include "Clava/FlatStream.h"
#include "llvm/Support/FileSystem.h"

#include <memory>
#include <string>
#include <system_error>

class Plugin : public DumpAstAction, public PluginASTAction {
  std::unique_ptr<llvm::raw_fd_ostream> output;
  std::unique_ptr<clava::flat::FlatStream> stream;

public:
  Plugin() { DumpResources::init(0, 0); }

  ~Plugin() override {
    if (stream) {
      stream->finish();
      stream.reset();
    }
    DumpResources::finish();
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    return DumpAstAction::CreateASTConsumer(CI, file);
  }

  bool ParseArgs(const CompilerInstance &, const std::vector<std::string> &args)
      override {
    std::string outputPath;
    for (const auto &argument : args) {
      if (argument.starts_with("-output=")) {
        outputPath = argument.substr(std::string("-output=").size());
      } else if (argument.starts_with("-file-id=")) {
        DumpResources::setRunId(
            std::stoi(argument.substr(std::string("-file-id=").size())));
      } else if (argument.starts_with("-system-header-threshold=")) {
        DumpResources::setSystemHeaderThreshold(std::stoi(
            argument.substr(std::string("-system-header-threshold=").size())));
      } else {
        llvm::errs() << "Unsupported DumpAst plugin argument: " << argument
                     << "\n";
        return false;
      }
    }

    if (outputPath.empty()) {
      llvm::errs() << "DumpAst requires -plugin-arg-DumpAst -output=<path>\n";
      return false;
    }

    std::error_code errorCode;
    output = std::make_unique<llvm::raw_fd_ostream>(
        outputPath, errorCode, llvm::sys::fs::OF_None);
    if (errorCode) {
      llvm::errs() << "Cannot open FlatBuffers output '" << outputPath
                   << "': " << errorCode.message() << "\n";
      return false;
    }

    clava::enableDenseIds();
    stream = std::make_unique<clava::flat::FlatStream>(*output);
    return true;
  }

  void EndSourceFileAction() override {
    if (stream) {
      stream->finish();
      stream.reset();
    }

    if (output) {
      output->flush();
      if (output->has_error()) {
        llvm::errs() << "Cannot write FlatBuffers output: "
                     << output->error().message() << "\n";
        output->clear_error();
      }
      output->close();
      if (output->has_error()) {
        llvm::errs() << "Cannot close FlatBuffers output: "
                     << output->error().message() << "\n";
        output->clear_error();
      }
    }
  }

  PluginASTAction::ActionType getActionType() override { return ReplaceAction; }
};

const static FrontendPluginRegistry::Add<Plugin> DumpAst(
    "DumpAst", "Writes the eager FlatBuffers AST protocol");
