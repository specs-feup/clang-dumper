//
// Created by JoaoBispo on 01/04/2018.
//

#include "../Clang/ClangNodes.h"
#include "../ClangAstDumper/ClangAstDumper.h"
#include "../ClangEnums/ClangEnums.h"
#include "../Clava/ClavaConstants.h"

#include <string>

// Selects the children visitor by class name. Several classes can share one
// visitor (e.g. all named casts use the ExplicitCastExpr visitor).
#define STMT_CHILDREN_ENTRY(CLASS, VISITOR)                                    \
  {#CLASS, [](ClangAstDumper &self, const Stmt *S,                             \
              std::vector<std::string> &children) {                            \
    self.VISITOR(static_cast<const CLASS *>(S), children);                     \
  }}

const std::map<std::string, ClangAstDumper::StmtChildrenFn>
    ClangAstDumper::STMT_CHILDREN_VISITORS = {
        STMT_CHILDREN_ENTRY(DeclStmt, VisitDeclStmtChildren),
        STMT_CHILDREN_ENTRY(IfStmt, VisitIfStmtChildren),
        STMT_CHILDREN_ENTRY(ForStmt, VisitForStmtChildren),
        STMT_CHILDREN_ENTRY(WhileStmt, VisitWhileStmtChildren),
        STMT_CHILDREN_ENTRY(DoStmt, VisitDoStmtChildren),
        STMT_CHILDREN_ENTRY(CXXForRangeStmt, VisitCXXForRangeStmtChildren),
        STMT_CHILDREN_ENTRY(CXXCatchStmt, VisitCXXCatchStmtChildren),
        STMT_CHILDREN_ENTRY(CXXTryStmt, VisitCXXTryStmtChildren),
        STMT_CHILDREN_ENTRY(CaseStmt, VisitCaseStmtChildren),
        STMT_CHILDREN_ENTRY(DefaultStmt, VisitDefaultStmtChildren),
        STMT_CHILDREN_ENTRY(GotoStmt, VisitGotoStmtChildren),
        STMT_CHILDREN_ENTRY(LabelStmt, VisitLabelStmtChildren),
        STMT_CHILDREN_ENTRY(AttributedStmt, VisitAttributedStmtChildren),
        STMT_CHILDREN_ENTRY(CapturedStmt, VisitCapturedStmtChildren),
};

// Selects the children visitor by class name. Classes absent from this table
// use the generic expression visitor (sub-statements plus type).
#define EXPR_CHILDREN_ENTRY(CLASS, VISITOR)                                    \
  {#CLASS, [](ClangAstDumper &self, const Expr *E,                             \
              std::vector<std::string> &children) {                            \
    self.VISITOR(static_cast<const CLASS *>(E), children);                     \
  }}

const std::map<std::string, ClangAstDumper::ExprChildrenFn>
    ClangAstDumper::EXPR_CHILDREN_VISITORS = {
        EXPR_CHILDREN_ENTRY(InitListExpr, VisitInitListExprChildren),
        EXPR_CHILDREN_ENTRY(DeclRefExpr, VisitDeclRefExprChildren),
        EXPR_CHILDREN_ENTRY(DependentScopeDeclRefExpr,
                            VisitDependentScopeDeclRefExprChildren),
        EXPR_CHILDREN_ENTRY(OffsetOfExpr, VisitOffsetOfExprChildren),
        EXPR_CHILDREN_ENTRY(MemberExpr, VisitMemberExprChildren),
        EXPR_CHILDREN_ENTRY(MaterializeTemporaryExpr,
                            VisitMaterializeTemporaryExprChildren),
        EXPR_CHILDREN_ENTRY(UnresolvedLookupExpr, VisitOverloadExprChildren),
        EXPR_CHILDREN_ENTRY(UnresolvedMemberExpr, VisitOverloadExprChildren),
        EXPR_CHILDREN_ENTRY(CallExpr, VisitCallExprChildren),
        EXPR_CHILDREN_ENTRY(CXXMemberCallExpr,
                            VisitCXXMemberCallExprChildren),
        EXPR_CHILDREN_ENTRY(CXXOperatorCallExpr, VisitCallExprChildren),
        EXPR_CHILDREN_ENTRY(UserDefinedLiteral, VisitCallExprChildren),
        EXPR_CHILDREN_ENTRY(CXXTypeidExpr, VisitCXXTypeidExprChildren),
        EXPR_CHILDREN_ENTRY(CStyleCastExpr, VisitExplicitCastExprChildren),
        EXPR_CHILDREN_ENTRY(CXXConstCastExpr, VisitExplicitCastExprChildren),
        EXPR_CHILDREN_ENTRY(CXXReinterpretCastExpr,
                            VisitExplicitCastExprChildren),
        EXPR_CHILDREN_ENTRY(CXXStaticCastExpr, VisitExplicitCastExprChildren),
        EXPR_CHILDREN_ENTRY(OpaqueValueExpr, VisitOpaqueValueExprChildren),
        EXPR_CHILDREN_ENTRY(CXXNewExpr, VisitCXXNewExprChildren),
        EXPR_CHILDREN_ENTRY(CXXDeleteExpr, VisitCXXDeleteExprChildren),
        EXPR_CHILDREN_ENTRY(LambdaExpr, VisitLambdaExprChildren),
        EXPR_CHILDREN_ENTRY(SizeOfPackExpr, VisitSizeOfPackExprChildren),
        EXPR_CHILDREN_ENTRY(UnaryExprOrTypeTraitExpr,
                            VisitUnaryExprOrTypeTraitExprChildren),
        EXPR_CHILDREN_ENTRY(DesignatedInitExpr,
                            VisitDesignatedInitExprChildren),
        EXPR_CHILDREN_ENTRY(CXXConstructExpr, VisitCXXConstructExprChildren),
        EXPR_CHILDREN_ENTRY(CXXTemporaryObjectExpr,
                            VisitCXXTemporaryObjectExprChildren),
        EXPR_CHILDREN_ENTRY(CXXDependentScopeMemberExpr,
                            VisitCXXDependentScopeMemberExprChildren),
        EXPR_CHILDREN_ENTRY(CXXPseudoDestructorExpr,
                            VisitCXXPseudoDestructorExprChildren),
        EXPR_CHILDREN_ENTRY(MSPropertyRefExpr,
                            VisitMSPropertyRefExprChildren),
};

void ClangAstDumper::visitChildren(const Stmt *S) {
    auto it = STMT_CHILDREN_VISITORS.find(clava::getClassName(S));

    std::vector<std::string> visitedChildren;
    if (it != STMT_CHILDREN_VISITORS.end()) {
        it->second(*this, S, visitedChildren);
    } else {
        VisitStmtChildren(S, visitedChildren);
    }

    dumpVisitedChildren(S, visitedChildren);
}

void ClangAstDumper::visitChildren(const Expr *E) {
    auto it = EXPR_CHILDREN_VISITORS.find(clava::getClassName(E));

    std::vector<std::string> visitedChildren;
    if (it != EXPR_CHILDREN_VISITORS.end()) {
        it->second(*this, E, visitedChildren);
    } else {
        VisitExprChildren(E, visitedChildren);
    }

    dumpVisitedChildren(E, visitedChildren);
}

void ClangAstDumper::VisitStmtChildren(const Stmt *S,
                                       std::vector<std::string> &children) {
    // Visit Stmt children
    for (const Stmt *SubStmt : S->children()) {
        if (SubStmt) {
            addChild(SubStmt, children);
        }
    }
}

void ClangAstDumper::VisitDeclStmtChildren(const DeclStmt *S,
                                           std::vector<std::string> &children) {
    // Do not visit sub-statements, only decls

    // Visit decls
    for (DeclStmt::const_decl_iterator I = S->decl_begin(), E = S->decl_end();
         I != E; ++I) {
        addChild(*I, children);
    }
}

void ClangAstDumper::VisitIfStmtChildren(const IfStmt *S,
                                         std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getConditionVariable(), children);
    addChild(S->getCond(), children);
    addChild(S->getThen(), children);
    addChild(S->getElse(), children);
}

void ClangAstDumper::VisitForStmtChildren(const ForStmt *S,
                                          std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the for stmts in a
    // controlled manner

    addChild(S->getInit(), children);
    addChild(S->getCond(), children);
    addChild(S->getInc(), children);
    addChild(S->getBody(), children);
    addChild(S->getConditionVariable(), children);
}

void ClangAstDumper::VisitWhileStmtChildren(
    const WhileStmt *S, std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getConditionVariable(), children);
    addChild(S->getCond(), children);
    addChild(S->getBody(), children);
}

void ClangAstDumper::VisitDoStmtChildren(const DoStmt *S,
                                         std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getBody(), children);
    addChild(S->getCond(), children);
}

void ClangAstDumper::VisitCXXForRangeStmtChildren(
    const CXXForRangeStmt *S, std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getRangeStmt(), children);
    addChild(S->getBeginStmt(), children);
    addChild(S->getEndStmt(), children);
    addChild(S->getCond(), children);
    addChild(S->getInc(), children);
    addChild(S->getLoopVarStmt(), children);
    addChild(S->getBody(), children);
}

void ClangAstDumper::VisitCXXCatchStmtChildren(
    const CXXCatchStmt *S, std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getExceptionDecl(), children);
    addChild(S->getHandlerBlock(), children);
}

void ClangAstDumper::VisitCXXTryStmtChildren(
    const CXXTryStmt *S, std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getTryBlock(), children);
    for (unsigned i = 0; i < S->getNumHandlers(); i++) {
        addChild(S->getHandler(i), children);
    }
}

void ClangAstDumper::VisitCaseStmtChildren(const CaseStmt *S,
                                           std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getLHS(), children);
    addChild(S->getRHS(), children);
    addChild(S->getSubStmt(), children);
}

void ClangAstDumper::VisitDefaultStmtChildren(
    const DefaultStmt *S, std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getSubStmt(), children);
}

void ClangAstDumper::VisitGotoStmtChildren(const GotoStmt *S,
                                           std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    VisitDeclTop(S->getLabel());
}

void ClangAstDumper::VisitLabelStmtChildren(
    const LabelStmt *S, std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getSubStmt(), children);

    VisitDeclTop(S->getDecl());
}

void ClangAstDumper::VisitAttributedStmtChildren(
    const AttributedStmt *S, std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    // Visit attributes
    for (auto attr : S->getAttrs()) {
        VisitAttrTop(attr);
        dumpTopLevelAttr(attr);
    }

    addChild(S->getSubStmt(), children);
}

void ClangAstDumper::VisitCapturedStmtChildren(
    const CapturedStmt *S, std::vector<std::string> &children) {
    // Do not visit sub-statements automatically, visit the if stmts in a
    // controlled manner

    addChild(S->getCapturedStmt(), children);
}

void ClangAstDumper::VisitExprChildren(const Expr *E,
                                       std::vector<std::string> &children) {
    // Visit sub-statements
    VisitStmtChildren(E, children);

    // Visit type
    VisitTypeTop(E->getType());
    dumpTopLevelType(E->getType());
}

void ClangAstDumper::VisitInitListExprChildren(
    const InitListExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    // Visit array filler
    VisitStmtTop(E->getArrayFiller());

    // Visit syntatic form
    VisitStmtTop(E->getSyntacticForm());
    VisitStmtTop(E->getSemanticForm());
}

void ClangAstDumper::VisitDeclRefExprChildren(
    const DeclRefExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    VisitDeclTop(E->getDecl());

    auto templateArgs = E->getTemplateArgs();
    for (unsigned i = 0; i < E->getNumTemplateArgs(); ++i) {
        auto templateArg = templateArgs + i;
        VisitTemplateArgument(templateArg->getArgument());
    }
}

void ClangAstDumper::VisitDependentScopeDeclRefExprChildren(
    const DependentScopeDeclRefExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    auto templateArgs = E->getTemplateArgs();
    for (unsigned i = 0; i < E->getNumTemplateArgs(); ++i) {
        auto templateArg = templateArgs + i;
        VisitTemplateArgument(templateArg->getArgument());
    }
}

void ClangAstDumper::VisitOffsetOfExprChildren(
    const OffsetOfExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    // Visit type
    VisitTypeTop(E->getTypeSourceInfo()->getType().getTypePtr());

    for (unsigned i = 0; i < E->getNumComponents(); i++) {
        // Dump each component
        OffsetOfNode node = E->getComponent(i);
        switch (node.getKind()) {
        case OffsetOfNode::Kind::Array:
            VisitStmtTop(E->getIndexExpr(node.getArrayExprIndex()));
            break;
        case OffsetOfNode::Kind::Identifier:
        case OffsetOfNode::Kind::Field:
            // Nothing to visit
            break;
        default:
            clava::throwNotImplemented(
                "ClangDataDumper::VisitOffsetOfExprChildren()",
                clava::OFFSET_OF_NODE_KIND[node.getKind()], Context,
                E->getSourceRange());
        }
    }
}

void ClangAstDumper::VisitMemberExprChildren(
    const MemberExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    // Visit decls
    VisitDeclTop(E->getMemberDecl());
    VisitDeclTop(E->getFoundDecl().getDecl());
}

void ClangAstDumper::VisitMaterializeTemporaryExprChildren(
    const MaterializeTemporaryExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    // Visit type
    VisitDeclTop(E->getExtendingDecl());
}

void ClangAstDumper::VisitOverloadExprChildren(
    const OverloadExpr *E, std::vector<std::string> &children) {
    // Hierarchy - direct parent is OverloadExpr
    VisitExprChildren(E, children);

    // Visit decls
    auto currentDecl = E->decls_begin(), declsEnd = E->decls_end();
    for (; currentDecl != declsEnd; ++currentDecl) {
        VisitDeclTop(*currentDecl);
    }

    // Visit template arguments
    auto templateArgs = E->getTemplateArgs();
    for (unsigned i = 0; i < E->getNumTemplateArgs(); ++i) {
        auto templateArg = templateArgs + i;
        VisitTemplateArgument(templateArg->getArgument());
    }
}

void ClangAstDumper::VisitCallExprChildren(const CallExpr *E,
                                           std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);
    VisitDeclTop(E->getDirectCallee());
}

void ClangAstDumper::VisitCXXMemberCallExprChildren(
    const CXXMemberCallExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitCallExprChildren(E, children);

    VisitDeclTop(E->getMethodDecl());
}

void ClangAstDumper::VisitCXXTypeidExprChildren(
    const CXXTypeidExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    if (E->isTypeOperand()) {
        VisitTypeTop(E->getTypeOperand(*Context));
    } else {
        VisitStmtTop(E->getExprOperand());
    }
}

void ClangAstDumper::VisitExplicitCastExprChildren(
    const ExplicitCastExpr *E, std::vector<std::string> &children) {
    // Hierarchy - direct parent is CastExpr
    VisitExprChildren(E, children);

    VisitTypeTop(E->getTypeAsWritten());
}

void ClangAstDumper::VisitOpaqueValueExprChildren(
    const OpaqueValueExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    addChild(E->getSourceExpr(), children);
}

void ClangAstDumper::VisitUnaryExprOrTypeTraitExprChildren(
    const UnaryExprOrTypeTraitExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    if (E->isArgumentType()) {
        VisitTypeTop(E->getArgumentType());
    }
}

void ClangAstDumper::VisitCXXNewExprChildren(
    const CXXNewExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    VisitStmtTop(E->getInitializer());
    VisitStmtTop(E->getArraySize().value_or(nullptr));
    VisitDeclTop(E->getOperatorNew());
}

void ClangAstDumper::VisitCXXDeleteExprChildren(
    const CXXDeleteExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    addChild(E->getArgument(), children);
}

void ClangAstDumper::VisitLambdaExprChildren(
    const LambdaExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    VisitDeclTop(E->getLambdaClass());
}

void ClangAstDumper::VisitSizeOfPackExprChildren(
    const SizeOfPackExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    VisitDeclTop(E->getPack());

    if (E->isPartiallySubstituted()) {
        for (auto templateArg : E->getPartialArguments()) {
            VisitTemplateArgument(templateArg);
        }
    }
}

void ClangAstDumper::VisitDesignatedInitExprChildren(
    const DesignatedInitExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);
}

void ClangAstDumper::VisitCXXConstructExprChildren(
    const CXXConstructExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    VisitDeclTop(E->getConstructor());
}

void ClangAstDumper::VisitCXXTemporaryObjectExprChildren(
    const CXXTemporaryObjectExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitCXXConstructExprChildren(E, children);
}

void ClangAstDumper::VisitCXXDependentScopeMemberExprChildren(
    const CXXDependentScopeMemberExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    auto templateArgs = E->getTemplateArgs();
    for (unsigned i = 0; i < E->getNumTemplateArgs(); ++i) {
        auto templateArg = templateArgs + i;
        VisitTemplateArgument(templateArg->getArgument());
    }
}

void ClangAstDumper::VisitCXXPseudoDestructorExprChildren(
    const CXXPseudoDestructorExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    VisitTypeTop(E->getDestroyedType());
}

void ClangAstDumper::VisitMSPropertyRefExprChildren(
    const MSPropertyRefExpr *E, std::vector<std::string> &children) {
    // Hierarchy
    VisitExprChildren(E, children);

    VisitExpr(E->getBaseExpr());
    VisitDeclTop(E->getPropertyDecl());
}
