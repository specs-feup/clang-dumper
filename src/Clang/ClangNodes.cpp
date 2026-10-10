//
// Created by JoaoBispo on 21/03/2018.
//

#include "ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"

#include "clang/AST/Attr.h"
#include "clang/Basic/OperatorKinds.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"

#include <bitset>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace clang;

namespace {

SourceLocation resolveTokenSplitLocation(const SourceManager &SM,
                                         SourceLocation loc) {
    while (loc.isMacroID()) {
        unsigned offset = SM.getDecomposedLoc(loc).second;
        CharSourceRange expansionRange = SM.getImmediateExpansionRange(loc);

        // Clang represents a split token (for example the two '>' characters
        // in a nested template-id) with macro locations even though no C/C++
        // macro was involved. Unlike real macro expansions, these mappings use
        // a character range. Unwrap them while preserving the character offset.
        if (expansionRange.isTokenRange()) {
            break;
        }

        SourceLocation expansionLoc =
            expansionRange.getBegin().getLocWithOffset(offset);
        if (expansionLoc.isInvalid() || expansionLoc == loc) {
            break;
        }

        loc = expansionLoc;
    }

    return loc;
}

StringRef getLocationFilename(const SourceManager &SM, SourceLocation loc) {
    StringRef filename = SM.getFilename(loc);
    return filename.empty() ? SM.getBufferName(loc) : filename;
}

std::unordered_map<const void *, uint32_t> &denseIds() {
    static std::unordered_map<const void *, uint32_t> ids;
    return ids;
}


} // namespace

const std::string clava::getClassName(const Decl *D) {
    const std::string kindName = D->getDeclKindName();
    return kindName + "Decl";
}

const std::string clava::getClassName(const Stmt *S) {
    const std::string className = S->getStmtClassName();
    return className;
}

const std::string clava::getClassName(const Type *T) {
    const std::string kindName = T->getTypeClassName();
    return kindName + "Type";
}

const std::string clava::getAttrKind(const Attr *A) {
    {
        switch (A->getKind()) {
#define ATTR(X)                                                                \
    case attr::X:                                                              \
        return #X;                                                             \
        break;
#include "clang/Basic/AttrList.inc"
        }
    }

    return "<undefined_attribute>";
}

const std::string clava::getClassName(const Attr *A) {
    const std::string kindName = clava::getAttrKind(A);

    return kindName + "Attr";
}

/** Dense id of a node within the current translation unit, numbered from 1 in visiting order. */
int64_t clava::getId(const void *addr, int id) {
    if (addr == nullptr) {
        return NULL_GENERIC;
    }
    auto &ids = denseIds();
    return ids.try_emplace(addr, static_cast<uint32_t>(ids.size() + 1)).first->second;
}

void clava::resetDenseIds() { denseIds().clear(); }

size_t clava::denseIdCount() { return denseIds().size(); }

int64_t clava::getId(const Decl *addr, int id) { return addr ? getId((const void *)addr, id) : NULL_DECL; }
int64_t clava::getId(const Stmt *addr, int id) { return addr ? getId((const void *)addr, id) : NULL_STMT; }
int64_t clava::getId(const Expr *addr, int id) { return addr ? getId((const void *)addr, id) : NULL_EXPR; }
int64_t clava::getId(std::optional<const Expr *> addr, int id) {
    return addr.has_value() ? getId(addr.value(), id) : NULL_EXPR;
}
int64_t clava::getId(const Type *addr, int id) { return addr ? getId((const void *)addr, id) : NULL_TYPE; }
int64_t clava::getId(const QualType &addr, int id) {
    return addr.isNull() ? NULL_TYPE : getId(addr.getAsOpaquePtr(), id);
}
int64_t clava::getId(const Attr *addr, int id) { return addr ? getId((const void *)addr, id) : NULL_ATTR; }

namespace {

/**
 * Builds the source text corresponding to the given source range, without
 * emitting anything to llvm::errs().
 */
const std::string buildSourceText(const SourceManager &sm,
                                  SourceRange sourceRange) {
    SourceLocation begin = sourceRange.getBegin();
    SourceLocation end = sourceRange.getEnd();
    if (begin.isMacroID()) {
        begin = sm.getSpellingLoc(begin);
    } else {
    }

    if (end.isMacroID()) {
        end = sm.getSpellingLoc(end);
    } else {
    }

    std::string text =
        Lexer::getSourceText(CharSourceRange::getTokenRange(begin, end), sm,
                             LangOptions(), 0)
            .str();
    if (text.size() > 0 &&
        (text.at(text.size() - 1) == ',')) { // the text can be ""
        return Lexer::getSourceText(CharSourceRange::getCharRange(begin, end),
                                    sm, LangOptions(), 0)
            .str();
    }

    return text;
}
} // namespace

/**
 * Taken from:
 * https://stackoverflow.com/questions/11083066/getting-the-source-behind-clangs-ast
 * @param Context
 * @param start
 * @param end
 * @return
 */
const std::string clava::getSourceText(ASTContext *Context,
                                       SourceRange sourceRange) {
    return buildSourceText(Context->getSourceManager(), sourceRange);
}

bool clava::isSystemHeader(const Stmt *S, ASTContext *Context) {
    FullSourceLoc fullLocation = Context->getFullLoc(S->getBeginLoc());
    return fullLocation.isValid() && fullLocation.isInSystemHeader();
}

bool clava::isSystemHeader(const Decl *D, ASTContext *Context) {
    FullSourceLoc fullLocation = Context->getFullLoc(D->getBeginLoc());
    return fullLocation.isValid() && fullLocation.isInSystemHeader();
}

/**
 * Taken from here:
 * https://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c#874160
 *
 * @param str
 * @param suffix
 * @return
 */
static bool endsWith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

const std::string clava::getQualifiedPrefix(const NamedDecl *D) {
    const std::string qualifiedName = D->getQualifiedNameAsString();
    const std::string declName = D->getDeclName().getAsString();

    // If declName is empty, return empty string
    if (declName.empty()) {
        return "";
    }

    // If declName is the same as the qualified name, return empty string
    if (declName == qualifiedName) {
        return "";
    }

    // Remove decl name and :: from qualified name
    const std::string expectedSuffix = "::" + declName;
    if (!endsWith(qualifiedName, expectedSuffix)) {
        std::string message = "ClangNodes::getQualifiedPrefix(const NamedDecl "
                              "*): Expected string '" +
                              qualifiedName + "' to have the suffix '" +
                              expectedSuffix + "'";
        throw std::invalid_argument(message);
    }

    int endIndex = qualifiedName.length() - declName.length() - 2;

    return qualifiedName.substr(0, endIndex);
}

void clava::throwNotImplemented(const std::string &source,
                                const std::string &caseNotImplemented,
                                ASTContext *Context, SourceLocation startLoc,
                                SourceLocation endLoc) {
    const SourceManager &SM = Context->getSourceManager();

    std::string locationInfo;
    SourceLocation startSpellingLoc = SM.getSpellingLoc(startLoc);
    if (startSpellingLoc.isValid()) {
        locationInfo = " at " + getLocationFilename(SM, startSpellingLoc).str() +
                       ":" + std::to_string(SM.getSpellingLineNumber(startSpellingLoc)) +
                       ":" + std::to_string(SM.getSpellingColumnNumber(startSpellingLoc));
    } else {
        locationInfo = " at <invalid>";
        SourceLocation endSpellingLoc = SM.getSpellingLoc(endLoc);
        if (endSpellingLoc.isValid()) {
            locationInfo = " at " + getLocationFilename(SM, endSpellingLoc).str() +
                           ":" + std::to_string(SM.getSpellingLineNumber(endSpellingLoc)) +
                           ":" + std::to_string(SM.getSpellingColumnNumber(endSpellingLoc));
        }
    }

    throw std::invalid_argument(source + ": Case not implemented, '" +
                                caseNotImplemented + "'" + locationInfo);
}

void clava::throwNotImplemented(const std::string &source,
                                const std::string &caseNotImplemented,
                                ASTContext *Context, SourceRange range) {
    const SourceManager &sm = Context->getSourceManager();
    throw std::invalid_argument(source + ": Case not implemented, '" +
                                caseNotImplemented +
                                "', source code that triggered the problem:\n" +
                                buildSourceText(sm, range));
}
