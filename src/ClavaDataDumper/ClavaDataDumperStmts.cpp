//
// Created by JoaoBispo on 30/03/2018.
//

#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"
#include "../ClavaDataDumper/ClavaDataDumper.h"

#include "llvm/ADT/STLForwardCompat.h"

#include <map>
#include "../Clava/HandlerCoverage.h"

// Data dumper selected directly by class name. Most entries dump with a
// method named after their own class; entries whose data section differs use
// the *_AS variants.
#define STMT_DATA_ENTRY(CLASS)                                                 \
  {#CLASS, {#CLASS, [](clava::ClavaDataDumper &self, const Stmt *S) {          \
    self.Dump##CLASS##Data(static_cast<const CLASS *>(S));                     \
  }}}

#define EXPR_DATA_ENTRY(CLASS)                                                 \
  {#CLASS, {#CLASS, [](clava::ClavaDataDumper &self, const Expr *E) {          \
    self.Dump##CLASS##Data(static_cast<const CLASS *>(E));                     \
  }}}

#define EXPR_DATA_ENTRY_AS(CLASS, SECTION)                                     \
  {#CLASS, {#SECTION, [](clava::ClavaDataDumper &self, const Expr *E) {        \
    self.Dump##SECTION##Data(static_cast<const CLASS *>(E));                   \
  }}}

const std::map<std::string, clava::ClavaDataDumper::StmtDataEntry>
    clava::ClavaDataDumper::STMT_DATA_DUMPERS = {
        STMT_DATA_ENTRY(LabelStmt),
        STMT_DATA_ENTRY(GotoStmt),
        STMT_DATA_ENTRY(AttributedStmt),
        STMT_DATA_ENTRY(GCCAsmStmt),
        STMT_DATA_ENTRY(MSAsmStmt),
};

const std::map<std::string, clava::ClavaDataDumper::ExprDataEntry>
    clava::ClavaDataDumper::EXPR_DATA_DUMPERS = {
        EXPR_DATA_ENTRY(CharacterLiteral),
        EXPR_DATA_ENTRY(IntegerLiteral),
        EXPR_DATA_ENTRY(FloatingLiteral),
        EXPR_DATA_ENTRY(CastExpr),
        EXPR_DATA_ENTRY_AS(CXXFunctionalCastExpr, CastExpr),
        EXPR_DATA_ENTRY_AS(CStyleCastExpr, ExplicitCastExpr),
        EXPR_DATA_ENTRY_AS(CXXAddrspaceCastExpr, CXXNamedCastExpr),
        EXPR_DATA_ENTRY_AS(CXXConstCastExpr, CXXNamedCastExpr),
        EXPR_DATA_ENTRY_AS(CXXDynamicCastExpr, CXXNamedCastExpr),
        EXPR_DATA_ENTRY_AS(CXXReinterpretCastExpr, CXXNamedCastExpr),
        EXPR_DATA_ENTRY_AS(CXXStaticCastExpr, CXXNamedCastExpr),
        EXPR_DATA_ENTRY(CXXBoolLiteralExpr),
        EXPR_DATA_ENTRY(CompoundLiteralExpr),
        EXPR_DATA_ENTRY(InitListExpr),
        EXPR_DATA_ENTRY(StringLiteral),
        EXPR_DATA_ENTRY(DeclRefExpr),
        EXPR_DATA_ENTRY(DependentScopeDeclRefExpr),
        EXPR_DATA_ENTRY(UnresolvedLookupExpr),
        EXPR_DATA_ENTRY(UnresolvedMemberExpr),
        EXPR_DATA_ENTRY(CXXConstructExpr),
        EXPR_DATA_ENTRY(CXXTemporaryObjectExpr),
        EXPR_DATA_ENTRY(MemberExpr),
        EXPR_DATA_ENTRY(MaterializeTemporaryExpr),
        EXPR_DATA_ENTRY(BinaryOperator),
        EXPR_DATA_ENTRY(UnaryOperator),
        EXPR_DATA_ENTRY_AS(CompoundAssignOperator, BinaryOperator),
        EXPR_DATA_ENTRY(CallExpr),
        EXPR_DATA_ENTRY(CXXMemberCallExpr),
        EXPR_DATA_ENTRY_AS(CXXOperatorCallExpr, CallExpr),
        EXPR_DATA_ENTRY_AS(UserDefinedLiteral, CallExpr),
        EXPR_DATA_ENTRY(CXXTypeidExpr),
        EXPR_DATA_ENTRY(CXXDependentScopeMemberExpr),
        EXPR_DATA_ENTRY(UnaryExprOrTypeTraitExpr),
        EXPR_DATA_ENTRY(CXXNewExpr),
        EXPR_DATA_ENTRY(CXXDeleteExpr),
        EXPR_DATA_ENTRY(OffsetOfExpr),
        EXPR_DATA_ENTRY(LambdaExpr),
        EXPR_DATA_ENTRY(PredefinedExpr),
        EXPR_DATA_ENTRY(SizeOfPackExpr),
        EXPR_DATA_ENTRY(ArrayInitLoopExpr),
        EXPR_DATA_ENTRY(DesignatedInitExpr),
        EXPR_DATA_ENTRY(CXXNoexceptExpr),
        EXPR_DATA_ENTRY(CXXPseudoDestructorExpr),
        EXPR_DATA_ENTRY(PseudoObjectExpr),
        EXPR_DATA_ENTRY(MSPropertyRefExpr),
};

void clava::ClavaDataDumper::dump(const Stmt *S) {
    const std::string classname = clava::getClassName(S);
    auto it = STMT_DATA_DUMPERS.find(classname);
    const char *dataName =
        it != STMT_DATA_DUMPERS.end() ? it->second.dataName : "Stmt";
    clava::recordHandlerEncounter("stmt data", classname);

    // Dump header
    llvm::errs() << "<" << dataName << "Data>\n";
    llvm::errs() << clava::getId(S, id) << "\n";
    llvm::errs() << clava::getClassName(S) << "\n";

    if (it != STMT_DATA_DUMPERS.end()) {
        it->second.dump(*this, S);
    } else {
        clava::recordHandlerFallback("stmt data", classname);
        // Default: plain Stmt data
        DumpStmtData(S);
    }
}

void clava::ClavaDataDumper::dump(const Expr *E) {
    const std::string classname = clava::getClassName(E);
    auto it = EXPR_DATA_DUMPERS.find(classname);
    const char *dataName =
        it != EXPR_DATA_DUMPERS.end() ? it->second.dataName : "Expr";
    clava::recordHandlerEncounter("expr data", classname);

    // Dump header
    llvm::errs() << "<" << dataName << "Data>\n";
    llvm::errs() << clava::getId(E, id) << "\n";
    llvm::errs() << clava::getClassName(E) << "\n";

    if (it != EXPR_DATA_DUMPERS.end()) {
        it->second.dump(*this, E);
    } else {
        clava::recordHandlerFallback("expr data", classname);
        // Default: plain Expr data
        DumpExprData(static_cast<const Expr *>(E));
    }
}

// STMTS

void clava::ClavaDataDumper::DumpStmtData(const Stmt *S) {

    // Original source range
    clava::dumpSourceInfo(Context, S->getBeginLoc(), S->getEndLoc());
}

void clava::ClavaDataDumper::DumpLabelStmtData(const LabelStmt *S) {
    // Hierarchy
    DumpStmtData(S);

    clava::dump(clava::getId(S->getDecl(), id));
}

void clava::ClavaDataDumper::DumpGotoStmtData(const GotoStmt *S) {
    // Hierarchy
    DumpStmtData(S);

    clava::dump(clava::getId(S->getLabel(), id));
}

void clava::ClavaDataDumper::DumpAttributedStmtData(const AttributedStmt *S) {
    // Hierarchy
    DumpStmtData(S);

    // Attributes
    std::vector<std::string> attributesIds;
    for (auto attr : S->getAttrs()) {
        attributesIds.push_back(clava::getId(attr, id));
    }
    clava::dump(attributesIds);
}

void clava::ClavaDataDumper::DumpAsmStmtData(const AsmStmt *S) {
    // Hierarchy
    DumpStmtData(S);

    clava::dump(S->isSimple());
    clava::dump(S->isVolatile());

    clava::dump(S->getNumClobbers());
    for (unsigned i = 0; i < S->getNumClobbers(); i++) {
        clava::dump(S->getClobber(i));
    }

    // Dump outputs
    clava::dump(S->getNumOutputs());
    for (unsigned i = 0; i < S->getNumOutputs(); i++) {
        clava::dump(getId(S->getOutputExpr(i), id));
        clava::dump(S->getOutputConstraint(i));
        clava::dump(S->isOutputPlusConstraint(i));
    }

    // Dump inputs
    clava::dump(S->getNumInputs());
    for (unsigned i = 0; i < S->getNumInputs(); i++) {
        clava::dump(getId(S->getInputExpr(i), id));
        clava::dump(S->getInputConstraint(i));
    }
}

void clava::ClavaDataDumper::DumpGCCAsmStmtData(const GCCAsmStmt *S) {
    // Hierarchy
    DumpAsmStmtData(S);

    clava::dump(S->generateAsmString((ASTContext &)*this->Context));
}

void clava::ClavaDataDumper::DumpMSAsmStmtData(const MSAsmStmt *S) {
    // Hierarchy
    DumpAsmStmtData(S);

    clava::dump(S->generateAsmString((ASTContext &)*this->Context));
}

// EXPRS

void clava::ClavaDataDumper::DumpExprData(const Expr *E) {
    DumpStmtData(E);

    clava::dump(E->getType(), id);
    clava::dump(E->getValueKind());
    clava::dump(E->getObjectKind());
    clava::dump(E->isDefaultArgument());
}

void clava::ClavaDataDumper::DumpCastExprData(const CastExpr *E) {
    DumpExprData(E);

    clava::dump(clava::CAST_KIND[E->getCastKind()]);
}

void clava::ClavaDataDumper::DumpLiteralData(const Expr *E) {
    DumpExprData(E);

    // Source literal
    clava::dump(clava::getSource(Context, E->getSourceRange()));
}

void clava::ClavaDataDumper::DumpCharacterLiteralData(
    const CharacterLiteral *E) {
    DumpLiteralData(E);

    clava::dump(E->getValue());
    clava::dump(clava::CHARACTER_LITERAL_KIND[llvm::to_underlying(
        E->getKind())]);
}

void clava::ClavaDataDumper::DumpIntegerLiteralData(const IntegerLiteral *E) {
    DumpLiteralData(E);
    bool isSigned = E->getType()->isSignedIntegerType();

    SmallString<0> str;
    E->getValue().toString(str, 10, isSigned);
    clava::dump(str);
}

void clava::ClavaDataDumper::DumpFloatingLiteralData(const FloatingLiteral *E) {
    DumpLiteralData(E);

    clava::dump(E->getValueAsApproximateDouble());
}

void clava::ClavaDataDumper::DumpStringLiteralData(const StringLiteral *E) {
    DumpLiteralData(E);

    clava::dump(clava::STRING_KIND[llvm::to_underlying(E->getKind())]);
    clava::dump(E->getLength());
    clava::dump(E->getCharByteWidth());

    clava::dump(E->getByteLength());
    for (auto currentByte = E->getBytes().bytes_begin(),
              lastByte = E->getBytes().bytes_end();
         currentByte != lastByte; ++currentByte) {
        clava::dump(*currentByte);
    }
}

void clava::ClavaDataDumper::DumpCXXBoolLiteralExprData(
    const CXXBoolLiteralExpr *E) {
    DumpLiteralData(E);

    clava::dump(E->getValue());
}

void clava::ClavaDataDumper::DumpCompoundLiteralExprData(
    const CompoundLiteralExpr *E) {
    DumpLiteralData(E);

    clava::dump(E->isFileScope());
}

void clava::ClavaDataDumper::DumpInitListExprData(const InitListExpr *E) {
    DumpExprData(E);

    clava::dump(clava::getId(E->getArrayFiller(), id));

    clava::dump(const_cast<InitListExpr *>(E)
                    ->isExplicit());       // isExplicit() could be const
    clava::dump(E->isStringLiteralInit()); // isExplicit() could be const
    clava::dump(clava::getId(E->getSyntacticForm(), id));
    clava::dump(clava::getId(E->getSemanticForm(), id));
}

void clava::ClavaDataDumper::DumpDeclRefExprData(const DeclRefExpr *E) {
    DumpExprData(E);

    // Dump qualifier
    clava::dump(E->getQualifier(), Context);

    // Dump template arguments
    if (E->hasExplicitTemplateArgs()) {
        // Number of template args
        clava::dump(E->getNumTemplateArgs());

        auto templateArgs = E->getTemplateArgs();
        for (unsigned i = 0; i < E->getNumTemplateArgs(); ++i) {
            auto templateArg = templateArgs + i;
            clava::dump(templateArg->getArgument(), id, Context);
        }
    } else {
        clava::dump(0);
    }

    clava::dump(clava::getId(E->getDecl(), id));
}

void clava::ClavaDataDumper::DumpDependentScopeDeclRefExprData(
    const DependentScopeDeclRefExpr *E) {
    DumpExprData(E);

    clava::dump(E->getDeclName().getAsString());

    // Dump qualifier
    clava::dump(E->getQualifier(), Context);

    clava::dump(E->hasTemplateKeyword());

    // Dump template arguments
    if (E->hasExplicitTemplateArgs()) {
        // Number of template args
        clava::dump(E->getNumTemplateArgs());

        auto templateArgs = E->getTemplateArgs();
        for (unsigned i = 0; i < E->getNumTemplateArgs(); ++i) {
            auto templateArg = templateArgs + i;
            clava::dump(templateArg->getArgument(), id, Context);
        }
    } else {
        clava::dump(0);
    }
}

void clava::ClavaDataDumper::DumpOverloadExprData(const OverloadExpr *E) {
    DumpExprData(E);

    // Dump qualifier
    clava::dump(E->getQualifier(), Context);

    // Name
    clava::dump(
        [&E](llvm::raw_string_ostream &stream) { stream << E->getName(); });

    // Number of decls
    clava::dump(E->getNumDecls());
    auto currentDecl = E->decls_begin(), declsEnd = E->decls_end();
    for (; currentDecl != declsEnd; ++currentDecl) {
        clava::dump(clava::getId(*currentDecl, id));
    }

    // Dump template arguments
    if (E->hasExplicitTemplateArgs()) {
        // Number of template args
        clava::dump(E->getNumTemplateArgs());

        auto templateArgs = E->getTemplateArgs();
        for (unsigned i = 0; i < E->getNumTemplateArgs(); ++i) {
            auto templateArg = templateArgs + i;
            clava::dump(templateArg->getArgument(), id, Context);
        }
    } else {
        clava::dump(0);
    }
}

void clava::ClavaDataDumper::DumpUnresolvedMemberExprData(
    const UnresolvedMemberExpr *E) {
    DumpOverloadExprData(E);
}

void clava::ClavaDataDumper::DumpUnresolvedLookupExprData(
    const UnresolvedLookupExpr *E) {
    DumpOverloadExprData(E);

    clava::dump(E->requiresADL());
}

void clava::ClavaDataDumper::DumpCXXConstructExprData(
    const CXXConstructExpr *E) {
    DumpExprData(E);

    // Dump qualifier
    clava::dump(E->isElidable());
    clava::dump(E->requiresZeroInitialization());
    clava::dump(E->isListInitialization());
    clava::dump(E->isStdInitListInitialization());
    clava::dump(clava::CONSTRUCTION_KIND[llvm::to_underlying(
        E->getConstructionKind())]);
    // Taken from here: http://codergears.com/Blog/?p=328
    clava::dump(
        E->isTemporaryObject(*Context, E->getConstructor()->getParent()));
    clava::dump(clava::getId(E->getConstructor(), id));
}

void clava::ClavaDataDumper::DumpCXXTemporaryObjectExprData(
    const CXXTemporaryObjectExpr *E) {
    DumpCXXConstructExprData(E);
}

void clava::ClavaDataDumper::DumpMemberExprData(const MemberExpr *E) {
    DumpExprData(E);

    clava::dump(E->isArrow());
    clava::dump(E->getMemberNameInfo().getAsString());
    clava::dump(clava::getId(E->getMemberDecl(), id));

    // Found decl
    clava::dump(clava::getId(E->getFoundDecl().getDecl(), id));
    clava::dump(ACCESS_SPECIFIER[E->getFoundDecl().getAccess()]);
}

void clava::ClavaDataDumper::DumpMaterializeTemporaryExprData(
    const MaterializeTemporaryExpr *E) {
    DumpExprData(E);

    clava::dump(getId(E->getExtendingDecl(), id));
}

void clava::ClavaDataDumper::DumpBinaryOperatorData(const BinaryOperator *E) {
    DumpExprData(E);

    clava::dump(clava::BINARY_OPERATOR_KIND[E->getOpcode()]);
}

void clava::ClavaDataDumper::DumpCallExprData(const CallExpr *E) {
    DumpExprData(E);

    clava::dump(clava::getId(E->getDirectCallee(), id));
}

void clava::ClavaDataDumper::DumpCXXMemberCallExprData(
    const CXXMemberCallExpr *E) {
    DumpCallExprData(E);

    clava::dump(clava::getId(E->getMethodDecl(), id));
}

void clava::ClavaDataDumper::DumpCXXTypeidExprData(const CXXTypeidExpr *E) {
    DumpExprData(E);

    clava::dump(E->isTypeOperand());
    if (E->isTypeOperand()) {
        clava::dump(clava::getId(E->getTypeOperand(*Context), id));
    } else {
        clava::dump(getId(E->getExprOperand(), id));
    }
}

void clava::ClavaDataDumper::DumpExplicitCastExprData(
    const ExplicitCastExpr *E) {
    DumpCastExprData(E);

    clava::dump(clava::getId(E->getTypeAsWritten(), id));
}

void clava::ClavaDataDumper::DumpCXXNamedCastExprData(
    const CXXNamedCastExpr *E) {
    DumpExplicitCastExprData(E);

    clava::dump(E->getCastName());
}

void clava::ClavaDataDumper::DumpCXXDependentScopeMemberExprData(
    const CXXDependentScopeMemberExpr *E) {
    DumpExprData(E);

    clava::dump(E->isArrow());
    clava::dump(E->getMemberNameInfo().getAsString());

    clava::dump(E->isImplicitAccess());
    clava::dump(E->getQualifier(), Context);
    clava::dump(E->hasTemplateKeyword());

    // Dump template arguments
    if (E->hasExplicitTemplateArgs()) {
        // Number of template args
        clava::dump(E->getNumTemplateArgs());

        auto templateArgs = E->getTemplateArgs();
        for (unsigned i = 0; i < E->getNumTemplateArgs(); ++i) {
            auto templateArg = templateArgs + i;
            clava::dump(templateArg->getArgument(), id, Context);
        }
    } else {
        clava::dump(0);
    }
}

void clava::ClavaDataDumper::DumpUnaryOperatorData(const UnaryOperator *E) {
    DumpExprData(E);

    clava::dump(clava::UNARY_OPERATOR_KIND[E->getOpcode()]);
    if (E->isPostfix()) {
        clava::dump("POSTFIX");
    } else {
        clava::dump("PREFIX");
    }
}

void clava::ClavaDataDumper::DumpUnaryExprOrTypeTraitExprData(
    const UnaryExprOrTypeTraitExpr *E) {
    DumpExprData(E);

    clava::dump(clava::UETT_KIND[E->getKind()]);
    clava::dump(E->isArgumentType());
    if (E->isArgumentType()) {
        clava::dump(getId(E->getArgumentType(), id));
    } else {
        clava::dump(getId((const Type *)nullptr, id));
    }

    clava::dump(clava::getSource(Context, E->getSourceRange()));
}

void clava::ClavaDataDumper::DumpCXXNewExprData(const CXXNewExpr *E) {
    DumpExprData(E);

    clava::dump(E->isGlobalNew());
    clava::dump(E->isArray());
    clava::dump(E->hasInitializer());
    clava::dump(clava::NEW_INIT_STYLE[llvm::to_underlying(
        E->getInitializationStyle())]);
    clava::dump(clava::getId(E->getInitializer(), id));
    clava::dump(clava::getId(E->getConstructExpr(), id));
    clava::dump(clava::getId(E->getArraySize(), id));
    clava::dump(clava::getId(E->getOperatorNew(), id));
}

void clava::ClavaDataDumper::DumpCXXDeleteExprData(const CXXDeleteExpr *E) {
    DumpExprData(E);

    clava::dump(E->isGlobalDelete());
    clava::dump(E->isArrayForm());
    clava::dump(E->isArrayFormAsWritten());
}

void clava::ClavaDataDumper::DumpOffsetOfExprData(const OffsetOfExpr *E) {
    DumpExprData(E);

    clava::dump(clava::getId(E->getTypeSourceInfo()->getType(), id));
    clava::dump(E->getNumComponents());
    for (unsigned i = 0; i < E->getNumComponents(); i++) {
        // Dump each component
        OffsetOfNode node = E->getComponent(i);
        clava::dump(clava::OFFSET_OF_NODE_KIND[node.getKind()]);

        switch (node.getKind()) {
        case OffsetOfNode::Kind::Array:
            clava::dump(
                clava::getId(E->getIndexExpr(node.getArrayExprIndex()), id));
            break;
        case OffsetOfNode::Kind::Base:
            clava::dump(clava::getId(node.getBase()->getType(), id));
            break;
        case OffsetOfNode::Kind::Identifier:
        case OffsetOfNode::Kind::Field:
            clava::dump(node.getFieldName()->getName().str());
            break;
        default:
            clava::throwNotImplemented(
                "ClangDataDumper::DumpOffsetOfExprData()",
                clava::OFFSET_OF_NODE_KIND[node.getKind()], Context,
                E->getSourceRange());
        }
    }
}

void clava::ClavaDataDumper::DumpLambdaExprData(const LambdaExpr *E) {
    DumpExprData(E);

    clava::dump(E->isGenericLambda());
    clava::dump(E->isMutable());
    clava::dump(E->hasExplicitParameters());
    clava::dump(E->hasExplicitResultType());
    clava::dump(clava::LAMBDA_CAPTURE_DEFAULT[E->getCaptureDefault()]);

    clava::dump(clava::getId(E->getLambdaClass(), id));

    clava::dump(E->capture_size());
    for (auto capture : E->captures()) {
        clava::dump(clava::LAMBDA_CAPTURE_KIND[capture.getCaptureKind()]);
    }
}

void clava::ClavaDataDumper::DumpPredefinedExprData(const PredefinedExpr *E) {
    DumpExprData(E);

    clava::dump(clava::PREDEFINED_ID_TYPE[llvm::to_underlying(
        E->getIdentKind())]);
}

void clava::ClavaDataDumper::DumpSizeOfPackExprData(const SizeOfPackExpr *E) {
    DumpExprData(E);

    clava::dump(E->isPartiallySubstituted());
    clava::dump(clava::getId(E->getPack(), id));

    if (E->isPartiallySubstituted()) {
        // Template args
        clava::dumpSize(E->getPartialArguments().size());
        for (auto partialArg : E->getPartialArguments()) {
            clava::dump(partialArg, id, Context);
        }
    } else {
        clava::dump(0);
    }
}

void clava::ClavaDataDumper::DumpArrayInitLoopExprData(
    const ArrayInitLoopExpr *E) {
    DumpExprData(E);
}

void clava::ClavaDataDumper::DumpDesignatedInitExprData(
    const DesignatedInitExpr *E) {
    DumpExprData(E);

    clava::dump(E->usesGNUSyntax());

    // Dump designators
    clava::dump(E->size());
    for (unsigned int i = 0; i < E->size(); i++) {
        clava::dump(E->getDesignator(i));
    }
}

void clava::ClavaDataDumper::DumpCXXNoexceptExprData(const CXXNoexceptExpr *E) {
    DumpExprData(E);

    clava::dump(E->getValue());
}

void clava::ClavaDataDumper::DumpCXXPseudoDestructorExprData(
    const CXXPseudoDestructorExpr *E) {
    DumpExprData(E);

    if (E->hasQualifier()) {
        clava::dump(E->getQualifier(), Context);
    } else {
        clava::dump("");
    }

    clava::dump(E->isArrow());
    clava::dump(clava::getId(E->getDestroyedType(), id));
}

void clava::ClavaDataDumper::DumpPseudoObjectExprData(
    const PseudoObjectExpr *E) {
    DumpExprData(E);

    auto resultExprIndex = E->getResultExprIndex();
    if (resultExprIndex == PseudoObjectExpr::NoResult) {
        clava::dump(-1);
    } else {
        clava::dump(E->getResultExprIndex());
    }
}

void clava::ClavaDataDumper::DumpMSPropertyRefExprData(
    const MSPropertyRefExpr *E) {
    DumpExprData(E);

    clava::dump(clava::getId(E->getBaseExpr(), id));
    clava::dump(clava::getId(E->getPropertyDecl(), id));
    clava::dump(E->isImplicitAccess());
    clava::dump(E->isArrow());
}
