//
// Created by JoaoBispo on 30/03/2018.
//

#include "../Clang/ClangNodes.h"
#include "../Clava/DumpStream.h"
#include "../Clava/HandlerCoverage.h"
#include "../ClangEnums/ClangEnums.h"
#include "../ClavaDataDumper/ClavaDataDumper.h"

#include "llvm/ADT/STLForwardCompat.h"

// Data dumper selected directly by class name. Most entries dump with a
// method named after their own class; entries whose data section differs use
// TYPE_DATA_ENTRY_AS(CLASS, SECTION).
#define TYPE_DATA_ENTRY(CLASS)                                                 \
  {#CLASS, {#CLASS, [](clava::ClavaDataDumper &self, const Type *T) {          \
    self.Dump##CLASS##Data(static_cast<const CLASS *>(T));                     \
  }}}

#define TYPE_DATA_ENTRY_AS(CLASS, SECTION)                                     \
  {#CLASS, {#SECTION, [](clava::ClavaDataDumper &self, const Type *T) {        \
    self.Dump##SECTION##Data(static_cast<const CLASS *>(T));                   \
  }}}

const std::map<std::string, clava::ClavaDataDumper::TypeDataEntry>
    clava::ClavaDataDumper::TYPE_DATA_DUMPERS = {
        TYPE_DATA_ENTRY(BuiltinType),
        TYPE_DATA_ENTRY(PointerType),
        TYPE_DATA_ENTRY(FunctionProtoType),
        TYPE_DATA_ENTRY_AS(FunctionNoProtoType, FunctionType),
        TYPE_DATA_ENTRY(ConstantArrayType),
        TYPE_DATA_ENTRY(VariableArrayType),
        TYPE_DATA_ENTRY_AS(IncompleteArrayType, ArrayType),
        TYPE_DATA_ENTRY(DependentSizedArrayType),
        TYPE_DATA_ENTRY_AS(RecordType, TagType),
        TYPE_DATA_ENTRY_AS(EnumType, TagType),
        TYPE_DATA_ENTRY(ElaboratedType),
        TYPE_DATA_ENTRY(TemplateTypeParmType),
        TYPE_DATA_ENTRY(TemplateSpecializationType),
        TYPE_DATA_ENTRY(TypedefType),
        TYPE_DATA_ENTRY(DecayedType),
        TYPE_DATA_ENTRY(DecltypeType),
        TYPE_DATA_ENTRY(AutoType),
        TYPE_DATA_ENTRY_AS(LValueReferenceType, ReferenceType),
        TYPE_DATA_ENTRY_AS(RValueReferenceType, ReferenceType),
        TYPE_DATA_ENTRY(TypeOfExprType),
        TYPE_DATA_ENTRY(PackExpansionType),
        TYPE_DATA_ENTRY(UnaryTransformType),
        TYPE_DATA_ENTRY(AttributedType),
        TYPE_DATA_ENTRY(SubstTemplateTypeParmType),
        TYPE_DATA_ENTRY(ComplexType),
};

void clava::ClavaDataDumper::dump(const Type *T) {
  const std::string classname = clava::getClassName(T);
  auto it = TYPE_DATA_DUMPERS.find(classname);
  const char *dataName =
      it != TYPE_DATA_DUMPERS.end() ? it->second.dataName : "Type";

  // Dump header
  clava::dumpStream() << "<" << dataName << "Data>\n";
  clava::dumpStream() << clava::getId(T, id) << "\n";
  clava::dumpStream() << clava::getClassName(T) << "\n";

  if (it != TYPE_DATA_DUMPERS.end()) {
    it->second.dump(*this, T);
  } else {
    clava::recordHandlerFallback("type data", classname);
    // Default: plain Type data
    DumpTypeData(T);
  }
}

void clava::ClavaDataDumper::DumpTypeData(const Type *T) {
  Qualifiers noQualifiers;
  DumpTypeData(T, noQualifiers);
}

/**
 * Data dumper for Type. To be used by both top-level QualType nodes and
 * unqualified types.
 * @param T
 */
void clava::ClavaDataDumper::DumpTypeData(const Type *T,
                                          Qualifiers &qualifiers) {
  clava::dump(
      QualType::getAsString(T, qualifiers, Context->getPrintingPolicy()));

  if (T->isDependentType()) {
    clava::dump("DEPENDENT");
  } else if (T->isInstantiationDependentType()) {
    clava::dump("INSTANTIATION_DEPENDENT");
  } else {
    clava::dump("NONE");
  }

  clava::dump(T->isVariablyModifiedType());
  clava::dump(T->containsUnexpandedParameterPack());
  clava::dump(T->isFromAST());

  QualType singleStepDesugar =
      T->getLocallyUnqualifiedSingleStepDesugaredType();
  if (singleStepDesugar != QualType(T, 0)) {
    clava::dump(clava::getId(singleStepDesugar, id));
  } else {
    clava::dump(clava::getId(((const Type *)nullptr), id));
  }
}

// Dumps the same information as DumpTypeData, and after that, information about
// QualType
void clava::ClavaDataDumper::dump(const QualType &T) {
  // Dump header
  clava::dumpStream() << "<QualTypeData>"
               << "\n";
  clava::dumpStream() << clava::getId(T, id) << "\n";
  clava::dumpStream() << "QualType"
               << "\n";

  auto qualifiers = T.getQualifiers();

  // Base type data
  DumpTypeData(T.getTypePtr(), qualifiers);

  // Dump C99 qualifiers
  clava::dump(qualifiers, Context);

  // Dumps address space
  LangAS addrspace = T.getAddressSpace();
  switch (addrspace) {
  case LangAS::Default:
    clava::dump("NONE");
    break;
  case LangAS::opencl_global:
    clava::dump("GLOBAL");
    break;
  case LangAS::opencl_local:
    clava::dump("LOCAL");
    break;
  case LangAS::opencl_constant:
    clava::dump("CONSTANT");
    break;
  case LangAS::opencl_generic:
    clava::dump("GENERIC");
    break;
  case LangAS::opencl_private:
    clava::dump("PRIVATE");
    break;

  case LangAS::cuda_constant:
    clava::dump("CUDA_CONSTANT");
    break;
  case LangAS::cuda_device:
    clava::dump("CUDA_DEVICE");
    break;
  case LangAS::cuda_shared:
    clava::dump("CUDA_SHARED");
    break;

  default:
    clava::dump("DEFAULT");
  }

  if (isTargetAddressSpace(addrspace)) {
    clava::dump(toTargetAddressSpace(addrspace));
  } else {
    clava::dump((unsigned)0);
  }

  // Unqualified type
  clava::dump(clava::getId(T.getTypePtr(), id));

  // TODO: The following code is valid but the ASTParser hasn't been updated
  // to handle it. Removing it for now as it breaks the whole tool.
  // Single desugar step
  // clava::dump(clava::getId(
  //     T.getSingleStepDesugaredType(*const_cast<const ASTContext *>(Context)),
  //     id));
}

void clava::ClavaDataDumper::DumpBuiltinTypeData(const BuiltinType *T) {
  DumpTypeData(T);

  clava::dump(clava::BUILTIN_KIND, T->getKind());
  clava::dump(T->getName(Context->getPrintingPolicy()));
}

void clava::ClavaDataDumper::DumpPointerTypeData(const PointerType *T) {
  DumpTypeData(T);

  clava::dump(clava::getId(T->getPointeeType(), id));
}

void clava::ClavaDataDumper::DumpFunctionTypeData(const FunctionType *T) {
  DumpTypeData(T);

  clava::dump(T->isConst());
  clava::dump(T->isVolatile());
  clava::dump(T->isRestrict());

  auto extInfo = T->getExtInfo();
  clava::dump(extInfo.getNoReturn());
  clava::dump(extInfo.getProducesResult());
  clava::dump(extInfo.getHasRegParm());
  clava::dump(extInfo.getHasRegParm() ? extInfo.getRegParm() : 0);
  clava::dump(clava::CALLING_CONVENTION[extInfo.getCC()]);

  clava::dump(clava::getId(T->getReturnType(), id));
}

void clava::ClavaDataDumper::DumpFunctionProtoTypeData(
    const FunctionProtoType *T) {
  DumpFunctionTypeData(T);

  // Num parameters
  clava::dumpSize(T->getParamTypes().size());

  // Parameters types
  clava::dumpSize(T->getParamTypes().size());
  for (QualType paramType : T->getParamTypes()) {
    clava::dump(clava::getId(paramType, id));
  }

  auto info = T->getExtProtoInfo();
  clava::dump(info.HasTrailingReturn);
  clava::dump(info.Variadic);

  clava::dump(clava::REFERENCE_QUALIFIER[info.RefQualifier]);

  clava::dump(clava::EXCEPTION_SPECIFICATION_TYPE[info.ExceptionSpec.Type]);

  // Dump types array
  clava::dumpSize(info.ExceptionSpec.Exceptions.size());
  for (auto &exceptType : info.ExceptionSpec.Exceptions) {
    clava::dump(clava::getId(exceptType, id));
  }

  switch (info.ExceptionSpec.Type) {
  case EST_DependentNoexcept:
    clava::dump(clava::getId(info.ExceptionSpec.NoexceptExpr, id));
    break;
  case EST_Unevaluated:
    clava::dump(clava::getId(info.ExceptionSpec.SourceDecl, id));
    break;
  case EST_Uninstantiated:
    clava::dump(clava::getId(info.ExceptionSpec.SourceDecl, id));
    clava::dump(clava::getId(info.ExceptionSpec.SourceTemplate, id));
    break;
  default:
    // No additional information required
    break;
  }
}

void clava::ClavaDataDumper::DumpTagTypeData(const TagType *T) {
  DumpTypeData(T);

  clava::dump(clava::getId(T->getDecl(), id));
}

void clava::ClavaDataDumper::DumpArrayTypeData(const ArrayType *T) {
  DumpTypeData(T);

  clava::dump(clava::ARRAY_SIZE_MODIFIER[llvm::to_underlying(
      T->getSizeModifier())]);

  // Dump C99 qualifiers of element type
  clava::dump(T->getIndexTypeQualifiers(), Context);
  clava::dump(clava::getId(T->getElementType(), id));
}

void clava::ClavaDataDumper::DumpConstantArrayTypeData(
    const ConstantArrayType *T) {
  // Hierarchy
  DumpArrayTypeData(T);

  SmallString<0> str;
  T->getSize().toString(str, 10, false);
  clava::dump(str);
}

void clava::ClavaDataDumper::DumpVariableArrayTypeData(
    const VariableArrayType *T) {
  // Hierarchy
  DumpArrayTypeData(T);

  clava::dump(clava::getId(T->getSizeExpr(), id));
}

void clava::ClavaDataDumper::DumpDependentSizedArrayTypeData(
    const DependentSizedArrayType *T) {
  // Hierarchy
  DumpArrayTypeData(T);

  clava::dump(clava::getId(T->getSizeExpr(), id));
}

void clava::ClavaDataDumper::DumpTypeWithKeywordData(const TypeWithKeyword *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(clava::ELABORATED_TYPE_KEYWORD[llvm::to_underlying(
      T->getKeyword())]);
}

void clava::ClavaDataDumper::DumpElaboratedTypeData(const ElaboratedType *T) {
  // Hierarchy
  DumpTypeWithKeywordData(T);

  clava::dump(T->getQualifier(), Context);
  clava::dump(clava::getId(T->getNamedType(), id));
}

void clava::ClavaDataDumper::DumpTemplateTypeParmTypeData(
    const TemplateTypeParmType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(T->getDepth());
  clava::dump(T->getIndex());
  clava::dump(T->isParameterPack());
  clava::dump(clava::getId(T->getDecl(), id));
}

void clava::ClavaDataDumper::DumpTemplateSpecializationTypeData(
    const TemplateSpecializationType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(T->isTypeAlias());
  if (T->isTypeAlias()) {
    clava::dump(clava::getId(T->getAliasedType(), id));
  } else {
    clava::dump(clava::getId((const Type *)nullptr, id));
  }

  clava::dump([&T](llvm::raw_string_ostream &stream) {
    T->getTemplateName().dump(stream);
  });
  clava::dump(clava::getId(T->getTemplateName().getAsTemplateDecl(), id));

  int numArgs = T->template_arguments().size();
  clava::dump(numArgs);
  for (auto &arg : T->template_arguments()) {
    clava::dump(arg, id, Context);
  }
}

void clava::ClavaDataDumper::DumpTypedefTypeData(const TypedefType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(clava::getId(T->getDecl(), id));
}

void clava::ClavaDataDumper::DumpAdjustedTypeData(const AdjustedType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(clava::getId(T->getOriginalType(), id));
  clava::dump(clava::getId(T->getAdjustedType(), id));
}

void clava::ClavaDataDumper::DumpDecayedTypeData(const DecayedType *T) {
  // Hierarchy
  DumpAdjustedTypeData(T);

  clava::dump(clava::getId(T->getDecayedType(), id));
  clava::dump(clava::getId(T->getPointeeType(), id));
}

void clava::ClavaDataDumper::DumpDecltypeTypeData(const DecltypeType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(T->isSugared());
  clava::dump(clava::getId(T->getUnderlyingExpr(), id));
}

void clava::ClavaDataDumper::DumpAutoTypeData(const AutoType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(clava::getId(T->getDeducedType(), id));
}

void clava::ClavaDataDumper::DumpReferenceTypeData(const ReferenceType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(clava::getId(T->getPointeeType(), id));
}

void clava::ClavaDataDumper::DumpPackExpansionTypeData(
    const PackExpansionType *T) {
  // Hierarchy
  DumpTypeData(T);

  if (T->getNumExpansions().has_value()) {
    clava::dump(T->getNumExpansions().value());
  } else {
    clava::dump(0);
  }

  clava::dump(clava::getId(T->getPattern(), id));
}

void clava::ClavaDataDumper::DumpTypeOfExprTypeData(const TypeOfExprType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(T->isSugared());
  clava::dump(clava::getId(T->getUnderlyingExpr(), id));
}

void clava::ClavaDataDumper::DumpAttributedTypeData(const AttributedType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(clava::getId(T->getModifiedType(), id));
  clava::dump(clava::getId(T->getEquivalentType(), id));
}

void clava::ClavaDataDumper::DumpUnaryTransformTypeData(
    const UnaryTransformType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(clava::UTT_KIND[T->getUTTKind()]);
  clava::dump(clava::getId(T->getUnderlyingType(), id));
  clava::dump(clava::getId(T->getBaseType(), id));
}

void clava::ClavaDataDumper::DumpSubstTemplateTypeParmTypeData(
    const SubstTemplateTypeParmType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(clava::getId(T->getReplacedParameter(), id));
  clava::dump(clava::getId(T->getReplacementType(), id));
}

void clava::ClavaDataDumper::DumpComplexTypeData(const ComplexType *T) {
  // Hierarchy
  DumpTypeData(T);

  clava::dump(clava::getId(T->getElementType(), id));
}
