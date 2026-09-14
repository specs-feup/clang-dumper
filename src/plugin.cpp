#include <clang/Frontend/FrontendPluginRegistry.h>

#include "Clang/ClangAst.h"
#include "Clava/ProtoStream.h"

#include <memory>

namespace {
std::unique_ptr<clava::proto::ProtoStream> &pluginOutput() {
  static std::unique_ptr<clava::proto::ProtoStream> output;
  return output;
}

size_t &pluginInstances() {
  static size_t instances = 0;
  return instances;
}
} // namespace

class Plugin : public DumpAstAction, public PluginASTAction {
public:
  Plugin() {
    if (!pluginOutput())
      pluginOutput() = std::make_unique<clava::proto::ProtoStream>(llvm::errs());
    ++pluginInstances();
    DumpResources::init(0, 0);
  }
  ~Plugin() override {
    DumpResources::finish();
    if (--pluginInstances() == 0) {
      pluginOutput()->finish();
      pluginOutput().reset();
    }
  }

  // Both DumpAstAction (via ASTFrontendAction) and PluginASTAction declare
  // CreateASTConsumer as a virtual method. PluginASTAction declares it as
  // pure virtual, so we must provide an implementation here. We delegate to
  // DumpAstAction's implementation which contains the actual AST consumer
  // creation logic shared with the standalone tool.
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    return DumpAstAction::CreateASTConsumer(CI, file);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    for (const auto &Arg : args) {
      if (Arg.find("-file-id=") == 0) {
        DumpResources::setRunId(std::stoi(Arg.substr(strlen("-file-id="))));
      } else if (Arg.find("-system-header-threshold=") == 0) {
        DumpResources::setSystemHeaderThreshold(
            std::stoi(Arg.substr(strlen("-system-header-threshold="))));
      }
    }

    return true; // Return true even if the argument is not found to
                 // continue execution
  }

  PluginASTAction::ActionType getActionType() override { return ReplaceAction; }
};

// Register the plugin with Clang
const static FrontendPluginRegistry::Add<Plugin>
    DumpAst("DumpAst", "Dumps the AST information to feed ClangStreamParserV2");
