#ifndef CLAVA_FLAT_SUPPORT_H
#define CLAVA_FLAT_SUPPORT_H

#include "complete_generated.h"
#include "FlatEnumSupport.h"
#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"
#include "clang/AST/AST.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Attrs.inc"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace clava::flat {
namespace fb = astwire::v2;
struct Context {
  clang::ASTContext *ast;
  int id;
  std::function<uint32_t(llvm::StringRef)> fileId;
};
int64_t wireId(const std::string &value);
std::unique_ptr<fb::NodeDataT> makeNodeData(clang::SourceLocation begin, clang::SourceLocation end, Context &c);
std::string qualifierString(clang::NestedNameSpecifier *qualifier, Context &c);
std::string sourceText(clang::SourceRange range, Context &c);
std::vector<fb::C99Qualifier> c99Qualifiers(clang::Qualifiers qualifiers, Context &c);
std::unique_ptr<fb::TemplateArgumentT> makeTemplateArgument(const clang::TemplateArgument &arg, Context &c);
std::unique_ptr<fb::TemplateNameT> makeTemplateName(const clang::TemplateName &name, Context &c);
std::unique_ptr<fb::CXXBaseSpecifierT> makeCXXBaseSpecifier(const clang::CXXBaseSpecifier &base, Context &c);
std::unique_ptr<fb::CXXCtorInitializerT> makeCXXCtorInitializer(const clang::CXXCtorInitializer *init, Context &c);
std::unique_ptr<fb::ExplicitSpecifierT> makeExplicitSpecifier(const clang::ExplicitSpecifier &specifier, Context &c);
std::unique_ptr<fb::ExceptionSpecificationT> makeExceptionSpecification(const clang::FunctionProtoType *type, Context &c);
std::unique_ptr<fb::OffsetOfComponentT> makeOffsetOfComponent(const clang::OffsetOfExpr *expr, unsigned index, Context &c);
std::unique_ptr<fb::DesignatorT> makeDesignator(const clang::DesignatedInitExpr::Designator *designator, Context &c);
std::unique_ptr<fb::NestedNameSpecifierT> makeNestedNameSpecifier(clang::NestedNameSpecifier *specifier, Context &c);

// Handler declarations are generated from the direct native definitions.
#include "FlatHandlerDeclarations.inc"
}
#endif
