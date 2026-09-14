inline obj::NodeT makeNode(const clang::Decl *node, Context &c) {
  obj::NodeT out; out.id = wireId(clava::getId(node, c.id)); out.class_name = clava::getClassName(node);
  if (out.class_name == "CXXConstructorDecl") { out.payload.Set(std::move(*makeCXXConstructorDeclData(static_cast<const clang::CXXConstructorDecl *>(node), c))); return out; }
  if (out.class_name == "CXXConversionDecl") { out.payload.Set(std::move(*makeCXXConversionDeclData(static_cast<const clang::CXXConversionDecl *>(node), c))); return out; }
  if (out.class_name == "CXXDestructorDecl") { out.payload.Set(std::move(*makeCXXMethodDeclData(static_cast<const clang::CXXDestructorDecl *>(node), c))); return out; }
  if (out.class_name == "CXXMethodDecl") { out.payload.Set(std::move(*makeCXXMethodDeclData(static_cast<const clang::CXXMethodDecl *>(node), c))); return out; }
  if (out.class_name == "FieldDecl") { out.payload.Set(std::move(*makeFieldDeclData(static_cast<const clang::FieldDecl *>(node), c))); return out; }
  if (out.class_name == "FunctionDecl") { out.payload.Set(std::move(*makeFunctionDeclData(static_cast<const clang::FunctionDecl *>(node), c))); return out; }
  if (out.class_name == "ObjCImplementationDecl") { out.payload.Set(std::move(*makeNamedDeclData(static_cast<const clang::ObjCImplementationDecl *>(node), c))); return out; }
  if (out.class_name == "ParmVarDecl") { out.payload.Set(std::move(*makeParmVarDeclData(static_cast<const clang::ParmVarDecl *>(node), c))); return out; }
  if (out.class_name == "ClassTemplateDecl") { out.payload.Set(std::move(*makeTemplateDeclData(static_cast<const clang::ClassTemplateDecl *>(node), c))); return out; }
  if (out.class_name == "FunctionTemplateDecl") { out.payload.Set(std::move(*makeTemplateDeclData(static_cast<const clang::FunctionTemplateDecl *>(node), c))); return out; }
  if (out.class_name == "TypeAliasTemplateDecl") { out.payload.Set(std::move(*makeTemplateDeclData(static_cast<const clang::TypeAliasTemplateDecl *>(node), c))); return out; }
  if (out.class_name == "VarTemplateDecl") { out.payload.Set(std::move(*makeTemplateDeclData(static_cast<const clang::VarTemplateDecl *>(node), c))); return out; }
  if (out.class_name == "TemplateTypeParmDecl") { out.payload.Set(std::move(*makeTemplateTypeParmDeclData(static_cast<const clang::TemplateTypeParmDecl *>(node), c))); return out; }
  if (out.class_name == "TypeDecl") { out.payload.Set(std::move(*makeTypeDeclData(static_cast<const clang::TypeDecl *>(node), c))); return out; }
  if (out.class_name == "UnresolvedUsingTypenameDecl") { out.payload.Set(std::move(*makeUnresolvedUsingTypenameDeclData(static_cast<const clang::UnresolvedUsingTypenameDecl *>(node), c))); return out; }
  if (out.class_name == "EnumDecl") { out.payload.Set(std::move(*makeEnumDeclData(static_cast<const clang::EnumDecl *>(node), c))); return out; }
  if (out.class_name == "RecordDecl") { out.payload.Set(std::move(*makeRecordDeclData(static_cast<const clang::RecordDecl *>(node), c))); return out; }
  if (out.class_name == "CXXRecordDecl") { out.payload.Set(std::move(*makeCXXRecordDeclData(static_cast<const clang::CXXRecordDecl *>(node), c))); return out; }
  if (out.class_name == "ClassTemplateSpecializationDecl") { out.payload.Set(std::move(*makeClassTemplateSpecializationDeclData(static_cast<const clang::ClassTemplateSpecializationDecl *>(node), c))); return out; }
  if (out.class_name == "ClassTemplatePartialSpecializationDecl") { out.payload.Set(std::move(*makeClassTemplatePartialSpecializationDeclData(static_cast<const clang::ClassTemplatePartialSpecializationDecl *>(node), c))); return out; }
  if (out.class_name == "VarDecl") { out.payload.Set(std::move(*makeVarDeclData(static_cast<const clang::VarDecl *>(node), c))); return out; }
  if (out.class_name == "EnumConstantDecl") { out.payload.Set(std::move(*makeValueDeclData(static_cast<const clang::EnumConstantDecl *>(node), c))); return out; }
  if (out.class_name == "NonTypeTemplateParmDecl") { out.payload.Set(std::move(*makeNonTypeTemplateParmDeclData(static_cast<const clang::NonTypeTemplateParmDecl *>(node), c))); return out; }
  if (out.class_name == "UsingShadowDecl") { out.payload.Set(std::move(*makeNamedDeclData(static_cast<const clang::UsingShadowDecl *>(node), c))); return out; }
  if (out.class_name == "TypeAliasDecl") { out.payload.Set(std::move(*makeTypedefNameDeclData(static_cast<const clang::TypeAliasDecl *>(node), c))); return out; }
  if (out.class_name == "TypedefDecl") { out.payload.Set(std::move(*makeTypedefNameDeclData(static_cast<const clang::TypedefDecl *>(node), c))); return out; }
  if (out.class_name == "AccessSpecDecl") { out.payload.Set(std::move(*makeAccessSpecDeclData(static_cast<const clang::AccessSpecDecl *>(node), c))); return out; }
  if (out.class_name == "UsingDirectiveDecl") { out.payload.Set(std::move(*makeUsingDirectiveDeclData(static_cast<const clang::UsingDirectiveDecl *>(node), c))); return out; }
  if (out.class_name == "NamespaceDecl") { out.payload.Set(std::move(*makeNamespaceDeclData(static_cast<const clang::NamespaceDecl *>(node), c))); return out; }
  if (out.class_name == "NamespaceAliasDecl") { out.payload.Set(std::move(*makeNamespaceAliasDeclData(static_cast<const clang::NamespaceAliasDecl *>(node), c))); return out; }
  if (out.class_name == "LinkageSpecDecl") { out.payload.Set(std::move(*makeLinkageSpecDeclData(static_cast<const clang::LinkageSpecDecl *>(node), c))); return out; }
  if (out.class_name == "LabelDecl") { out.payload.Set(std::move(*makeNamedDeclData(static_cast<const clang::LabelDecl *>(node), c))); return out; }
  if (out.class_name == "StaticAssertDecl") { out.payload.Set(std::move(*makeStaticAssertDeclData(static_cast<const clang::StaticAssertDecl *>(node), c))); return out; }
  if (out.class_name == "TemplateTemplateParmDecl") { out.payload.Set(std::move(*makeTemplateTemplateParmDeclData(static_cast<const clang::TemplateTemplateParmDecl *>(node), c))); return out; }
  if (out.class_name == "MSPropertyDecl") { out.payload.Set(std::move(*makeMSPropertyDeclData(static_cast<const clang::MSPropertyDecl *>(node), c))); return out; }
  if (out.class_name == "UsingDecl") { out.payload.Set(std::move(*makeUsingDeclData(static_cast<const clang::UsingDecl *>(node), c))); return out; }
  if (out.class_name == "VarTemplateSpecializationDecl") { out.payload.Set(std::move(*makeVarDeclData(static_cast<const clang::VarTemplateSpecializationDecl *>(node), c))); return out; }
  recordHandlerFallback("decl data", out.class_name);
  out.payload.Set(std::move(*makeDeclData(node, c))); return out;
}

inline obj::NodeT makeNode(const clang::Stmt *node, Context &c) {
  obj::NodeT out; out.id = wireId(clava::getId(node, c.id)); out.class_name = clava::getClassName(node);
  if (out.class_name == "LabelStmt") { out.payload.Set(std::move(*makeLabelStmtData(static_cast<const clang::LabelStmt *>(node), c))); return out; }
  if (out.class_name == "GotoStmt") { out.payload.Set(std::move(*makeGotoStmtData(static_cast<const clang::GotoStmt *>(node), c))); return out; }
  if (out.class_name == "AttributedStmt") { out.payload.Set(std::move(*makeAttributedStmtData(static_cast<const clang::AttributedStmt *>(node), c))); return out; }
  if (out.class_name == "GCCAsmStmt") { out.payload.Set(std::move(*makeGCCAsmStmtData(static_cast<const clang::GCCAsmStmt *>(node), c))); return out; }
  if (out.class_name == "MSAsmStmt") { out.payload.Set(std::move(*makeMSAsmStmtData(static_cast<const clang::MSAsmStmt *>(node), c))); return out; }
  recordHandlerFallback("stmt data", out.class_name);
  out.payload.Set(std::move(*makeStmtData(node, c))); return out;
}

inline obj::NodeT makeNode(const clang::Expr *node, Context &c) {
  obj::NodeT out; out.id = wireId(clava::getId(node, c.id)); out.class_name = clava::getClassName(node);
  if (out.class_name == "CharacterLiteral") { out.payload.Set(std::move(*makeCharacterLiteralData(static_cast<const clang::CharacterLiteral *>(node), c))); return out; }
  if (out.class_name == "IntegerLiteral") { out.payload.Set(std::move(*makeIntegerLiteralData(static_cast<const clang::IntegerLiteral *>(node), c))); return out; }
  if (out.class_name == "FloatingLiteral") { out.payload.Set(std::move(*makeFloatingLiteralData(static_cast<const clang::FloatingLiteral *>(node), c))); return out; }
  if (out.class_name == "CastExpr") { out.payload.Set(std::move(*makeCastExprData(static_cast<const clang::CastExpr *>(node), c))); return out; }
  if (out.class_name == "CXXFunctionalCastExpr") { out.payload.Set(std::move(*makeCastExprData(static_cast<const clang::CXXFunctionalCastExpr *>(node), c))); return out; }
  if (out.class_name == "CStyleCastExpr") { out.payload.Set(std::move(*makeExplicitCastExprData(static_cast<const clang::CStyleCastExpr *>(node), c))); return out; }
  if (out.class_name == "CXXAddrspaceCastExpr") { out.payload.Set(std::move(*makeCXXNamedCastExprData(static_cast<const clang::CXXAddrspaceCastExpr *>(node), c))); return out; }
  if (out.class_name == "CXXConstCastExpr") { out.payload.Set(std::move(*makeCXXNamedCastExprData(static_cast<const clang::CXXConstCastExpr *>(node), c))); return out; }
  if (out.class_name == "CXXDynamicCastExpr") { out.payload.Set(std::move(*makeCXXNamedCastExprData(static_cast<const clang::CXXDynamicCastExpr *>(node), c))); return out; }
  if (out.class_name == "CXXReinterpretCastExpr") { out.payload.Set(std::move(*makeCXXNamedCastExprData(static_cast<const clang::CXXReinterpretCastExpr *>(node), c))); return out; }
  if (out.class_name == "CXXStaticCastExpr") { out.payload.Set(std::move(*makeCXXNamedCastExprData(static_cast<const clang::CXXStaticCastExpr *>(node), c))); return out; }
  if (out.class_name == "CXXBoolLiteralExpr") { out.payload.Set(std::move(*makeCXXBoolLiteralExprData(static_cast<const clang::CXXBoolLiteralExpr *>(node), c))); return out; }
  if (out.class_name == "CompoundLiteralExpr") { out.payload.Set(std::move(*makeCompoundLiteralExprData(static_cast<const clang::CompoundLiteralExpr *>(node), c))); return out; }
  if (out.class_name == "InitListExpr") { out.payload.Set(std::move(*makeInitListExprData(static_cast<const clang::InitListExpr *>(node), c))); return out; }
  if (out.class_name == "StringLiteral") { out.payload.Set(std::move(*makeStringLiteralData(static_cast<const clang::StringLiteral *>(node), c))); return out; }
  if (out.class_name == "DeclRefExpr") { out.payload.Set(std::move(*makeDeclRefExprData(static_cast<const clang::DeclRefExpr *>(node), c))); return out; }
  if (out.class_name == "DependentScopeDeclRefExpr") { out.payload.Set(std::move(*makeDependentScopeDeclRefExprData(static_cast<const clang::DependentScopeDeclRefExpr *>(node), c))); return out; }
  if (out.class_name == "UnresolvedLookupExpr") { out.payload.Set(std::move(*makeUnresolvedLookupExprData(static_cast<const clang::UnresolvedLookupExpr *>(node), c))); return out; }
  if (out.class_name == "UnresolvedMemberExpr") { out.payload.Set(std::move(*makeUnresolvedMemberExprData(static_cast<const clang::UnresolvedMemberExpr *>(node), c))); return out; }
  if (out.class_name == "CXXConstructExpr") { out.payload.Set(std::move(*makeCXXConstructExprData(static_cast<const clang::CXXConstructExpr *>(node), c))); return out; }
  if (out.class_name == "CXXTemporaryObjectExpr") { out.payload.Set(std::move(*makeCXXTemporaryObjectExprData(static_cast<const clang::CXXTemporaryObjectExpr *>(node), c))); return out; }
  if (out.class_name == "MemberExpr") { out.payload.Set(std::move(*makeMemberExprData(static_cast<const clang::MemberExpr *>(node), c))); return out; }
  if (out.class_name == "MaterializeTemporaryExpr") { out.payload.Set(std::move(*makeMaterializeTemporaryExprData(static_cast<const clang::MaterializeTemporaryExpr *>(node), c))); return out; }
  if (out.class_name == "BinaryOperator") { out.payload.Set(std::move(*makeBinaryOperatorData(static_cast<const clang::BinaryOperator *>(node), c))); return out; }
  if (out.class_name == "UnaryOperator") { out.payload.Set(std::move(*makeUnaryOperatorData(static_cast<const clang::UnaryOperator *>(node), c))); return out; }
  if (out.class_name == "CompoundAssignOperator") { out.payload.Set(std::move(*makeBinaryOperatorData(static_cast<const clang::CompoundAssignOperator *>(node), c))); return out; }
  if (out.class_name == "CallExpr") { out.payload.Set(std::move(*makeCallExprData(static_cast<const clang::CallExpr *>(node), c))); return out; }
  if (out.class_name == "CXXMemberCallExpr") { out.payload.Set(std::move(*makeCXXMemberCallExprData(static_cast<const clang::CXXMemberCallExpr *>(node), c))); return out; }
  if (out.class_name == "CXXOperatorCallExpr") { out.payload.Set(std::move(*makeCallExprData(static_cast<const clang::CXXOperatorCallExpr *>(node), c))); return out; }
  if (out.class_name == "UserDefinedLiteral") { out.payload.Set(std::move(*makeCallExprData(static_cast<const clang::UserDefinedLiteral *>(node), c))); return out; }
  if (out.class_name == "CXXTypeidExpr") { out.payload.Set(std::move(*makeCXXTypeidExprData(static_cast<const clang::CXXTypeidExpr *>(node), c))); return out; }
  if (out.class_name == "CXXDependentScopeMemberExpr") { out.payload.Set(std::move(*makeCXXDependentScopeMemberExprData(static_cast<const clang::CXXDependentScopeMemberExpr *>(node), c))); return out; }
  if (out.class_name == "UnaryExprOrTypeTraitExpr") { out.payload.Set(std::move(*makeUnaryExprOrTypeTraitExprData(static_cast<const clang::UnaryExprOrTypeTraitExpr *>(node), c))); return out; }
  if (out.class_name == "CXXNewExpr") { out.payload.Set(std::move(*makeCXXNewExprData(static_cast<const clang::CXXNewExpr *>(node), c))); return out; }
  if (out.class_name == "CXXDeleteExpr") { out.payload.Set(std::move(*makeCXXDeleteExprData(static_cast<const clang::CXXDeleteExpr *>(node), c))); return out; }
  if (out.class_name == "OffsetOfExpr") { out.payload.Set(std::move(*makeOffsetOfExprData(static_cast<const clang::OffsetOfExpr *>(node), c))); return out; }
  if (out.class_name == "LambdaExpr") { out.payload.Set(std::move(*makeLambdaExprData(static_cast<const clang::LambdaExpr *>(node), c))); return out; }
  if (out.class_name == "PredefinedExpr") { out.payload.Set(std::move(*makePredefinedExprData(static_cast<const clang::PredefinedExpr *>(node), c))); return out; }
  if (out.class_name == "SizeOfPackExpr") { out.payload.Set(std::move(*makeSizeOfPackExprData(static_cast<const clang::SizeOfPackExpr *>(node), c))); return out; }
  if (out.class_name == "ArrayInitLoopExpr") { out.payload.Set(std::move(*makeArrayInitLoopExprData(static_cast<const clang::ArrayInitLoopExpr *>(node), c))); return out; }
  if (out.class_name == "DesignatedInitExpr") { out.payload.Set(std::move(*makeDesignatedInitExprData(static_cast<const clang::DesignatedInitExpr *>(node), c))); return out; }
  if (out.class_name == "CXXNoexceptExpr") { out.payload.Set(std::move(*makeCXXNoexceptExprData(static_cast<const clang::CXXNoexceptExpr *>(node), c))); return out; }
  if (out.class_name == "CXXPseudoDestructorExpr") { out.payload.Set(std::move(*makeCXXPseudoDestructorExprData(static_cast<const clang::CXXPseudoDestructorExpr *>(node), c))); return out; }
  if (out.class_name == "PseudoObjectExpr") { out.payload.Set(std::move(*makePseudoObjectExprData(static_cast<const clang::PseudoObjectExpr *>(node), c))); return out; }
  if (out.class_name == "MSPropertyRefExpr") { out.payload.Set(std::move(*makeMSPropertyRefExprData(static_cast<const clang::MSPropertyRefExpr *>(node), c))); return out; }
  recordHandlerFallback("expr data", out.class_name);
  out.payload.Set(std::move(*makeExprData(node, c))); return out;
}

inline obj::NodeT makeNode(const clang::Type *node, Context &c) {
  obj::NodeT out; out.id = wireId(clava::getId(node, c.id)); out.class_name = clava::getClassName(node);
  if (out.class_name == "BuiltinType") { out.payload.Set(std::move(*makeBuiltinTypeData(static_cast<const clang::BuiltinType *>(node), c))); return out; }
  if (out.class_name == "PointerType") { out.payload.Set(std::move(*makePointerTypeData(static_cast<const clang::PointerType *>(node), c))); return out; }
  if (out.class_name == "FunctionProtoType") { out.payload.Set(std::move(*makeFunctionProtoTypeData(static_cast<const clang::FunctionProtoType *>(node), c))); return out; }
  if (out.class_name == "FunctionNoProtoType") { out.payload.Set(std::move(*makeFunctionTypeData(static_cast<const clang::FunctionNoProtoType *>(node), c))); return out; }
  if (out.class_name == "ConstantArrayType") { out.payload.Set(std::move(*makeConstantArrayTypeData(static_cast<const clang::ConstantArrayType *>(node), c))); return out; }
  if (out.class_name == "VariableArrayType") { out.payload.Set(std::move(*makeVariableArrayTypeData(static_cast<const clang::VariableArrayType *>(node), c))); return out; }
  if (out.class_name == "IncompleteArrayType") { out.payload.Set(std::move(*makeArrayTypeData(static_cast<const clang::IncompleteArrayType *>(node), c))); return out; }
  if (out.class_name == "DependentSizedArrayType") { out.payload.Set(std::move(*makeDependentSizedArrayTypeData(static_cast<const clang::DependentSizedArrayType *>(node), c))); return out; }
  if (out.class_name == "RecordType") { out.payload.Set(std::move(*makeTagTypeData(static_cast<const clang::RecordType *>(node), c))); return out; }
  if (out.class_name == "EnumType") { out.payload.Set(std::move(*makeTagTypeData(static_cast<const clang::EnumType *>(node), c))); return out; }
  if (out.class_name == "ElaboratedType") { out.payload.Set(std::move(*makeElaboratedTypeData(static_cast<const clang::ElaboratedType *>(node), c))); return out; }
  if (out.class_name == "TemplateTypeParmType") { out.payload.Set(std::move(*makeTemplateTypeParmTypeData(static_cast<const clang::TemplateTypeParmType *>(node), c))); return out; }
  if (out.class_name == "TemplateSpecializationType") { out.payload.Set(std::move(*makeTemplateSpecializationTypeData(static_cast<const clang::TemplateSpecializationType *>(node), c))); return out; }
  if (out.class_name == "TypedefType") { out.payload.Set(std::move(*makeTypedefTypeData(static_cast<const clang::TypedefType *>(node), c))); return out; }
  if (out.class_name == "DecayedType") { out.payload.Set(std::move(*makeDecayedTypeData(static_cast<const clang::DecayedType *>(node), c))); return out; }
  if (out.class_name == "DecltypeType") { out.payload.Set(std::move(*makeDecltypeTypeData(static_cast<const clang::DecltypeType *>(node), c))); return out; }
  if (out.class_name == "AutoType") { out.payload.Set(std::move(*makeAutoTypeData(static_cast<const clang::AutoType *>(node), c))); return out; }
  if (out.class_name == "LValueReferenceType") { out.payload.Set(std::move(*makeReferenceTypeData(static_cast<const clang::LValueReferenceType *>(node), c))); return out; }
  if (out.class_name == "RValueReferenceType") { out.payload.Set(std::move(*makeReferenceTypeData(static_cast<const clang::RValueReferenceType *>(node), c))); return out; }
  if (out.class_name == "TypeOfExprType") { out.payload.Set(std::move(*makeTypeOfExprTypeData(static_cast<const clang::TypeOfExprType *>(node), c))); return out; }
  if (out.class_name == "PackExpansionType") { out.payload.Set(std::move(*makePackExpansionTypeData(static_cast<const clang::PackExpansionType *>(node), c))); return out; }
  if (out.class_name == "UnaryTransformType") { out.payload.Set(std::move(*makeUnaryTransformTypeData(static_cast<const clang::UnaryTransformType *>(node), c))); return out; }
  if (out.class_name == "AttributedType") { out.payload.Set(std::move(*makeAttributedTypeData(static_cast<const clang::AttributedType *>(node), c))); return out; }
  if (out.class_name == "SubstTemplateTypeParmType") { out.payload.Set(std::move(*makeSubstTemplateTypeParmTypeData(static_cast<const clang::SubstTemplateTypeParmType *>(node), c))); return out; }
  if (out.class_name == "ComplexType") { out.payload.Set(std::move(*makeComplexTypeData(static_cast<const clang::ComplexType *>(node), c))); return out; }
  recordHandlerFallback("type data", out.class_name);
  out.payload.Set(std::move(*makeTypeData(node, c))); return out;
}

inline obj::NodeT makeNode(const clang::Attr *node, Context &c) {
  obj::NodeT out; out.id = wireId(clava::getId(node, c.id)); out.class_name = clava::getClassName(node);
  if (out.class_name == "AlignedAttr") { out.payload.Set(std::move(*makeAlignedAttrData(static_cast<const clang::AlignedAttr *>(node), c))); return out; }
  if (out.class_name == "OpenCLUnrollHintAttr") { out.payload.Set(std::move(*makeOpenCLUnrollHintAttrData(static_cast<const clang::OpenCLUnrollHintAttr *>(node), c))); return out; }
  if (out.class_name == "FormatAttr") { out.payload.Set(std::move(*makeFormatAttrData(static_cast<const clang::FormatAttr *>(node), c))); return out; }
  if (out.class_name == "NonNullAttr") { out.payload.Set(std::move(*makeNonNullAttrData(static_cast<const clang::NonNullAttr *>(node), c))); return out; }
  if (out.class_name == "VisibilityAttr") { out.payload.Set(std::move(*makeVisibilityAttrData(static_cast<const clang::VisibilityAttr *>(node), c))); return out; }
  recordHandlerFallback("attr data", out.class_name);
  out.payload.Set(std::move(*makeAttributeData(node, c))); return out;
}

inline obj::NodeT makeNode(const clang::QualType &node, Context &c) {
  obj::NodeT out; out.id = wireId(clava::getId(node, c.id)); out.class_name = "QualType";
  out.payload.Set(std::move(*makeQualTypeData(node, c))); return out;
}

