#include "../Clava/ProtoStream.h"
#include <stdexcept>
#include <utility>
//
// Created by JoaoBispo on 20/01/2017.
//

#include "ClangAstDumper.h"

namespace obj = ::astwire::v1obj;
#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"

#include "clang/Lex/Lexer.h"

#include "clang/Basic/SourceManager.h"

// #define DEBUG

using namespace clang;

ClangAstDumper::ClangAstDumper(ASTContext *Context, int id,
                               int systemHeaderThreshold)
    : Context(Context), id(id), systemHeaderThreshold(systemHeaderThreshold){};

// This method is equivalent to a VisitQualType() in ClangAstDumperTypes.cpp
void ClangAstDumper::VisitTypeTop(const QualType &T) {

  if (T.isNull()) {
    return;
  }

  // Check if QualType is the same as the underlying type
  if ((void *)T.getTypePtr() == T.getAsOpaquePtr()) {
    // TODO: AST dump method relies on visiting the nodes multiple times
    // For now, detect it to avoid visiting children more than once
    if (seenTypes.count(T.getTypePtr()) == 0) {
      TypeVisitor::Visit(T.getTypePtr());
    }

    dumpType(T.getTypePtr());
    return;
  }

  if (dumpType(T)) {
    return;
  }

  visitChildren(T);
  if (!clava::proto::emit(T, Context, id))
    throw std::logic_error("protobuf AST stream is not active");
  dumpIdToClassMap(T.getAsOpaquePtr(), "QualType");

}

void ClangAstDumper::VisitTypeTop(const Type *T) {
  if (T == nullptr) {
    return;
  }

  TypeVisitor::Visit(T);
}

void ClangAstDumper::VisitStmtTop(const Stmt *Node) {
  if (Node == nullptr) {
    return;
  }

  ConstStmtVisitor::Visit(Node);
}

void ClangAstDumper::VisitDeclTop(const Decl *Node) {
  if (Node == nullptr) {
    return;
  }

  ConstDeclVisitor::Visit(Node);
}

void ClangAstDumper::VisitAttrTop(const Attr *Node) {
  if (Node == nullptr) {
    return;
  }

  VisitAttr(Node);
}

void ClangAstDumper::log(std::string name, const void *addr) {
#ifdef DEBUG
  llvm::errs() << name << " " << addr << "\n";
#endif
}

void ClangAstDumper::log(const Decl *D) { log(clava::getClassName(D), D); }

void ClangAstDumper::log(const Stmt *S) { log(clava::getClassName(S), S); }

void ClangAstDumper::log(const Type *T) { log(clava::getClassName(T), T); }

void ClangAstDumper::log(const Attr *A) { log(clava::getClassName(A), A); }

void ClangAstDumper::dumpVisitedChildren(const void *pointer,
                                         std::vector<std::string> children) {
  if (auto *stream = clava::proto::ProtoStream::active()) {
    obj::ChildrenT record;
    record.node = clava::proto::wireId(clava::getId(pointer, id));
    for (const auto &child : children)
      record.children.push_back(clava::proto::wireId(child));
    stream->record(record);
    return;
  }
  throw std::logic_error("protobuf AST stream is not active");
}

void ClangAstDumper::dumpIdToClassMap(const void *pointer,
                                      std::string className) {
  if (auto *stream = clava::proto::ProtoStream::active()) {
    obj::NodeClassT record;
    record.node = clava::proto::wireId(clava::getId(pointer, id));
    record.class_name = std::move(className);
    stream->record(record);
    return;
  }
  throw std::logic_error("protobuf AST stream is not active");
}

void ClangAstDumper::dumpTopLevelType(const QualType &type) {
  if (auto *stream = clava::proto::ProtoStream::active()) {
    obj::TopLevelT record;
    record.kind = obj::TopLevelKind::TOPLEVELKIND_TYPE;
    record.node = clava::proto::wireId(clava::getId(type, id));
    stream->record(record);
    return;
  }
  throw std::logic_error("protobuf AST stream is not active");
}

void ClangAstDumper::dumpTopLevelAttr(const Attr *attr) {
  if (auto *stream = clava::proto::ProtoStream::active()) {
    obj::TopLevelT record;
    record.kind = obj::TopLevelKind::TOPLEVELKIND_ATTR;
    record.node = clava::proto::wireId(clava::getId(attr, id));
    stream->record(record);
    return;
  }
  throw std::logic_error("protobuf AST stream is not active");
}

void ClangAstDumper::visitTemplateArguments(
    const TemplateArgumentLoc *templateArgs, unsigned count) {
  for (unsigned i = 0; i < count; ++i) {
    VisitTemplateArgument((templateArgs + i)->getArgument());
  }
}

bool ClangAstDumper::isPastSystemHeaderThreshold() const {
  // The level is incremented while visiting a system-header node, and this
  // predicate is checked at that node's outgoing child edges. Consequently,
  // a positive threshold N serializes the node at level N + 1 but suppresses
  // its children. Non-positive values disable structural thresholding.
  return systemHeaderThreshold > 0 &&
         currentSystemHeaderLevel > systemHeaderThreshold;
}

// Common implementation for all addChild() overloads: the only difference
// between them is which Top-level visit function serializes the node.
template <typename T, typename F>
void ClangAstDumper::addChildInternal(const T *addr,
                                      std::vector<std::string> &children,
                                      F &&visitTop) {
  if (isPastSystemHeaderThreshold()) {
    return;
  }

  std::string clavaId = clava::getId(addr, id);

  visitTop(addr);
  children.push_back(clavaId);
}

// Overload for QualType, which is passed by value/reference instead of pointer.
template <typename F>
void ClangAstDumper::addChildInternal(const QualType &addr,
                                      std::vector<std::string> &children,
                                      F &&visitTop) {
  if (isPastSystemHeaderThreshold()) {
    return;
  }

  std::string clavaId = clava::getId(addr, id);

  visitTop(addr);
  children.push_back(clavaId);
}

const void ClangAstDumper::addChild(const Decl *addr,
                                    std::vector<std::string> &children) {
  addChildInternal(addr, children,
                   [this](const Decl *D) { VisitDeclTop(D); });
};

const void ClangAstDumper::addChildren(DeclContext::decl_range decls,
                                       std::vector<std::string> &children) {

  for (auto decl = decls.begin(), endDecl = decls.end(); decl != endDecl;
       ++decl) {

    // Ignore decls that are not in the source code
    if (decl->isImplicit()) {
      continue;
    }

    if (*decl == nullptr) {
      continue;
    }

    addChild(*decl, children);
  }
};

const void ClangAstDumper::addChild(const Stmt *addr,
                                    std::vector<std::string> &children) {
  addChildInternal(addr, children,
                   [this](const Stmt *S) { VisitStmtTop(S); });
};

const void ClangAstDumper::addChild(const Expr *addr,
                                    std::vector<std::string> &children) {
  addChildInternal(addr, children,
                   [this](const Expr *E) { VisitStmtTop(E); });
};

const void ClangAstDumper::addChild(const Type *addr,
                                    std::vector<std::string> &children) {
  addChildInternal(addr, children,
                   [this](const Type *T) { VisitTypeTop(T); });
};

const void ClangAstDumper::addChild(const QualType &addr,
                                    std::vector<std::string> &children) {
  addChildInternal(addr, children,
                   [this](const QualType &T) { VisitTypeTop(T); });
};

const void ClangAstDumper::addChild(const Attr *addr,
                                    std::vector<std::string> &children) {
  addChildInternal(addr, children,
                   [this](const Attr *A) { VisitAttrTop(A); });
};

// Shared implementation of VisitTemplateArgument() and
// VisitTemplateArgChildren(). They differ in a single point: whether
// argument packs are expanded into their elements.
void ClangAstDumper::visitTemplateArgument(const TemplateArgument &templateArg,
                                           bool expandPacks) {
  switch (templateArg.getKind()) {
  case TemplateArgument::ArgKind::Type:
    VisitTypeTop(templateArg.getAsType());
    break;
  case TemplateArgument::ArgKind::Expression:
    VisitStmtTop(templateArg.getAsExpr());
    break;
  case TemplateArgument::ArgKind::Pack:
    if (expandPacks) {
      for (auto currentArg = templateArg.pack_begin(),
                endArg = templateArg.pack_end();
           currentArg != endArg; ++currentArg) {
        visitTemplateArgument(*currentArg, expandPacks);
      }
    }
    // Non-expanding visitors ignore packs
    break;
  case TemplateArgument::ArgKind::Integral:
    // Do nothing
    break;
  case TemplateArgument::ArgKind::Template:
    VisitTemplateName(templateArg.getAsTemplate());
    break;
  case TemplateArgument::ArgKind::Declaration:
    VisitDeclTop(templateArg.getAsDecl());
    break;
  case TemplateArgument::ArgKind::NullPtr:
    VisitTypeTop(templateArg.getNullPtrType().getTypePtr());
    break;
  case TemplateArgument::ArgKind::TemplateExpansion:
    VisitTemplateName(templateArg.getAsTemplateOrTemplatePattern());
    break;
  case TemplateArgument::ArgKind::StructuralValue:
    // Pass the QualType as-is: stripping qualifiers via getTypePtr() would
    // visit the unqualified type while the serializer emits the qualified
    // type's id, referencing a type never emitted.
    VisitTypeTop(templateArg.getStructuralValueType());
    break;
  default:
    throw std::invalid_argument(
        "ClangAstDumper::visitTemplateArgument(): Case not implemented, '" +
        clava::TEMPLATE_ARG_KIND[templateArg.getKind()] + "'");
  }
};

void ClangAstDumper::VisitTemplateArgument(
    const TemplateArgument &templateArg) {
  visitTemplateArgument(templateArg, /*expandPacks=*/false);
};

void ClangAstDumper::VisitTemplateArgChildren(
    const TemplateArgument &templateArg) {
  visitTemplateArgument(templateArg, /*expandPacks=*/true);
};

void ClangAstDumper::VisitTemplateName(const TemplateName &templateName) {

  switch (templateName.getKind()) {
  case TemplateName::NameKind::Template:
    VisitDeclTop(templateName.getAsTemplateDecl());
    break;
  case TemplateName::NameKind::QualifiedTemplate:
    VisitDeclTop(templateName.getAsTemplateDecl());
    break;
  case TemplateName::NameKind::SubstTemplateTemplateParm:
    VisitDeclTop(templateName.getAsSubstTemplateTemplateParm()->getParameter());
    VisitTemplateName(
        templateName.getAsSubstTemplateTemplateParm()->getReplacement());
    break;
  case TemplateName::NameKind::UsingTemplate:
    VisitDeclTop(templateName.getAsUsingShadowDecl());
    break;
  case TemplateName::NameKind::DependentTemplate:
    // A dependent template name (e.g. `T::template apply`) refers to no
    // declaration; there is nothing to visit.
    break;
  default:
    throw std::invalid_argument(
        "ClangAstDumper::VisitTemplateName(): TemplateName case not "
        "implemented, '" +
        clava::TEMPLATE_NAME_KIND[templateName.getKind()] + "'");
  }
};
