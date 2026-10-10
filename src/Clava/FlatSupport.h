#ifndef CLAVA_FLAT_SUPPORT_H
#define CLAVA_FLAT_SUPPORT_H

#include "flatbuffers/base.h"
#if FLATBUFFERS_VERSION_MAJOR != 25 || FLATBUFFERS_VERSION_MINOR != 12 || \
    FLATBUFFERS_VERSION_REVISION != 19
#error "clang-dumper requires FlatBuffers runtime 25.12.19"
#endif

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
  std::function<bool(clang::SourceLocation)> isInlineAsm;
};
std::unique_ptr<fb::ClavaNodeDataT> makeNodeData(clang::SourceLocation begin, clang::SourceLocation end, Context &c);
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
std::unique_ptr<fb::StmtDataT> makeStmtData(const clang::Stmt *stmt, Context &c);

/** A Node record with the node's wire id and Clang class name; the family's makeNode adds the payload. */
template <class T> fb::NodeT nodeHeader(const T *node, Context &c) {
  fb::NodeT out;
  out.id = clava::getId(node, c.id);
  out.class_name = clava::getClassName(node);
  return out;
}

/** Inside makeNode: nodes of Clang class {@code Class} get the payload built by {@code Builder}. */
#define FLAT_PAYLOAD(Class, Builder)                                                                     \
  if (out.class_name == #Class) {                                                                        \
    out.payload.Set(std::move(*Builder(static_cast<const clang::Class *>(node), c)));                    \
    return out;                                                                                          \
  }
}
#endif
