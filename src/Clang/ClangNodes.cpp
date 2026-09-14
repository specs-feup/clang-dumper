// Clang AST identity, naming and source-fidelity helpers shared by the
// protobuf emitters and child visitors.  The old text-stream value dumpers
// intentionally do not live here anymore.

#include "ClangNodes.h"

#include "clang/AST/Attr.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_map>

using namespace clang;

namespace {

StringRef getLocationFilename(const SourceManager &sm, SourceLocation loc) {
    StringRef filename = sm.getFilename(loc);
    return filename.empty() ? sm.getBufferName(loc) : filename;
}

std::unordered_map<const void *, uint64_t> &denseIds() {
    static std::unordered_map<const void *, uint64_t> ids;
    return ids;
}

bool useDenseIds = false;

const std::string buildSourceText(const SourceManager &sm,
                                  SourceRange sourceRange) {
    SourceLocation begin = sourceRange.getBegin();
    SourceLocation end = sourceRange.getEnd();
    if (begin.isMacroID())
        begin = sm.getSpellingLoc(begin);
    if (end.isMacroID())
        end = sm.getSpellingLoc(end);

    std::string text =
        Lexer::getSourceText(CharSourceRange::getTokenRange(begin, end), sm,
                             LangOptions(), 0)
            .str();
    if (!text.empty() && text.back() == ',')
        return Lexer::getSourceText(CharSourceRange::getCharRange(begin, end),
                                    sm, LangOptions(), 0)
            .str();
    return text;
}

} // namespace

const std::string clava::getClassName(const Decl *decl) {
    return std::string(decl->getDeclKindName()) + "Decl";
}

const std::string clava::getClassName(const Stmt *stmt) {
    return stmt->getStmtClassName();
}

const std::string clava::getClassName(const Type *type) {
    return std::string(type->getTypeClassName()) + "Type";
}

const std::string clava::getAttrKind(const Attr *attr) {
    switch (attr->getKind()) {
#define ATTR(X)                                                               \
    case attr::X:                                                             \
        return #X;
#include "clang/Basic/AttrList.inc"
    }
    return "<undefined_attribute>";
}

const std::string clava::getClassName(const Attr *attr) {
    return getAttrKind(attr) + "Attr";
}

const std::string clava::getId(const void *addr, int id) {
    if (addr == nullptr)
        return "0_" + std::to_string(id);

    if (useDenseIds) {
        auto &ids = denseIds();
        const auto [entry, inserted] =
            ids.try_emplace(addr, static_cast<uint64_t>(ids.size() + 1));
        (void)inserted;
        return "@" + std::to_string(entry->second);
    }

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%p_%d", addr, id);
    return buffer;
}

void clava::enableDenseIds() { useDenseIds = true; }
void clava::resetDenseIds() { denseIds().clear(); }
size_t clava::denseIdCount() { return denseIds().size(); }

const std::string clava::getId(const Decl *addr, int id) {
    return addr == nullptr ? "nullptr_decl" : getId(static_cast<const void *>(addr), id);
}

const std::string clava::getId(const Stmt *addr, int id) {
    return addr == nullptr ? "nullptr_stmt" : getId(static_cast<const void *>(addr), id);
}

const std::string clava::getId(const Expr *addr, int id) {
    return addr == nullptr ? "nullptr_expr" : getId(static_cast<const void *>(addr), id);
}

const std::string clava::getId(std::optional<const Expr *> addr, int id) {
    return addr.has_value() ? getId(addr.value(), id) : "nullptr_expr";
}

const std::string clava::getId(const Type *addr, int id) {
    return addr == nullptr ? "nullptr_type" : getId(static_cast<const void *>(addr), id);
}

const std::string clava::getId(const QualType &addr, int id) {
    return addr.isNull() ? "nullptr_type" : getId(addr.getAsOpaquePtr(), id);
}

const std::string clava::getId(const Attr *addr, int id) {
    return addr == nullptr ? "nullptr_attr" : getId(static_cast<const void *>(addr), id);
}

bool clava::isSystemHeader(const Stmt *stmt, ASTContext *context) {
    FullSourceLoc location = context->getFullLoc(stmt->getBeginLoc());
    return location.isValid() && location.isInSystemHeader();
}

bool clava::isSystemHeader(const Decl *decl, ASTContext *context) {
    FullSourceLoc location = context->getFullLoc(decl->getBeginLoc());
    return location.isValid() && location.isInSystemHeader();
}

static bool endsWith(const std::string &value, const std::string &suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

const std::string clava::getQualifiedPrefix(const NamedDecl *decl) {
    const std::string qualifiedName = decl->getQualifiedNameAsString();
    const std::string declName = decl->getDeclName().getAsString();
    if (declName.empty() || declName == qualifiedName)
        return "";

    const std::string suffix = "::" + declName;
    if (!endsWith(qualifiedName, suffix))
        throw std::invalid_argument("ClangNodes::getQualifiedPrefix: expected '" +
                                    qualifiedName + "' to end with '" + suffix + "'");
    return qualifiedName.substr(0, qualifiedName.size() - suffix.size());
}

void clava::throwNotImplemented(const std::string &source,
                                const std::string &caseNotImplemented,
                                ASTContext *context, SourceLocation startLoc,
                                SourceLocation endLoc) {
    const SourceManager &sm = context->getSourceManager();
    SourceLocation spelling = sm.getSpellingLoc(startLoc);
    std::string locationInfo;
    if (spelling.isValid()) {
        locationInfo = " at " + getLocationFilename(sm, spelling).str() + ":" +
                       std::to_string(sm.getSpellingLineNumber(spelling)) + ":" +
                       std::to_string(sm.getSpellingColumnNumber(spelling));
    } else {
        locationInfo = " at <invalid>";
    }
    throw std::invalid_argument(source + ": case not implemented, '" +
                                caseNotImplemented + "'" + locationInfo);
}

void clava::throwNotImplemented(const std::string &source,
                                const std::string &caseNotImplemented,
                                ASTContext *context, SourceRange range) {
    throw std::invalid_argument(source + ": case not implemented, '" +
                                caseNotImplemented +
                                "', source code that triggered the problem:\n" +
                                buildSourceText(context->getSourceManager(), range));
}
