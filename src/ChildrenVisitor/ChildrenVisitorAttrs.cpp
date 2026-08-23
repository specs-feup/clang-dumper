//
// Created by JoaoBispo on 12/04/2018.
//

#include "../Clang/ClangNodes.h"
#include "../ClangAstDumper/ClangAstDumper.h"
#include "../Clava/HandlerCoverage.h"

#include <string>

#define ATTR_CHILDREN_ENTRY(CLASS, VISITOR)                                    \
  {#CLASS, [](ClangAstDumper &self, const Attr *A,                             \
              std::vector<std::string> &children) {                            \
    self.VISITOR(static_cast<const CLASS *>(A), children);                     \
  }}

const std::map<std::string, ClangAstDumper::AttrChildrenFn>
    ClangAstDumper::ATTR_CHILDREN_VISITORS = {
        ATTR_CHILDREN_ENTRY(AlignedAttr, VisitAlignedAttrChildren),
};

void ClangAstDumper::visitChildren(const Attr *A) {
    const std::string classname = clava::getClassName(A);
    auto it = ATTR_CHILDREN_VISITORS.find(classname);

    std::vector<std::string> visitedChildren;
    if (it != ATTR_CHILDREN_VISITORS.end()) {
        it->second(*this, A, visitedChildren);
    } else {
        clava::recordHandlerFallback("attr children", classname);
    }
    // By default, attributes have no children to visit

    dumpVisitedChildren(A, visitedChildren);
}

void ClangAstDumper::VisitAlignedAttrChildren(
    const AlignedAttr *A, std::vector<std::string> &children) {
    // No hierarchy

    if (A->isAlignmentExpr()) {
        // TODO: This required dependency must bypass structural thresholding
        // until https://github.com/specs-feup/clang-dumper/issues/21 is fixed.
        const Expr *alignmentExpr = A->getAlignmentExpr();
        VisitStmtTop(alignmentExpr);
        children.push_back(clava::getId(alignmentExpr, id));
    } else {
        VisitTypeTop(A->getAlignmentType()->getType());
        dumpTopLevelType(A->getAlignmentType()->getType());
    }
}
