#include "ProtoSupport.h"

#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/STLForwardCompat.h"

namespace clava::proto {

namespace {

template <typename T> std::unique_ptr<T> typeBase(const clang::Type *type) {
  (void)type;
  return std::make_unique<T>();
}

std::string templateNameText(const clang::TemplateName &name) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  name.dump(stream);
  return text;
}

} // namespace

std::unique_ptr<obj::TypeDataT> makeTypeData(const clang::Type *type,
                                            Context &c,
                                            clang::Qualifiers qualifiers) {
  auto out = typeBase<obj::TypeDataT>(type);
  out->type_as_string =
      clang::QualType::getAsString(type, qualifiers, c.ast->getPrintingPolicy());
  if (type->isDependentType())
    out->type_dependency = enumValue<obj::TypeDependency>("DEPENDENT");
  else if (type->isInstantiationDependentType())
    out->type_dependency =
        enumValue<obj::TypeDependency>("INSTANTIATION_DEPENDENT");
  else
    out->type_dependency = enumValue<obj::TypeDependency>("NONE");
  out->is_variably_modified = type->isVariablyModifiedType();
  out->contains_unexpanded_parameter_pack =
      type->containsUnexpandedParameterPack();
  out->is_from_ast = type->isFromAST();
  clang::QualType desugared = type->getLocallyUnqualifiedSingleStepDesugaredType();
  out->unqualified_desugared_type = -1;
  if (desugared != clang::QualType(type, 0))
    out->unqualified_desugared_type =
        wireId(clava::getId(desugared, c.id));
  return out;
}

std::unique_ptr<obj::TypeDataT> makeTypeData(const clang::Type *type,
                                            Context &c) {
  return makeTypeData(type, c, clang::Qualifiers());
}

std::unique_ptr<obj::QualTypeDataT> makeQualTypeData(const clang::QualType &type,
                                                    Context &c) {
  auto out = std::make_unique<obj::QualTypeDataT>();
  auto qualifiers = type.getQualifiers();
  out->base = makeTypeData(type.getTypePtr(), c, qualifiers);
  out->c99_qualifiers = c99Qualifiers(qualifiers, c);

  clang::LangAS addressSpace = type.getAddressSpace();
  switch (addressSpace) {
  case clang::LangAS::Default:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("NONE");
    break;
  case clang::LangAS::opencl_global:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("GLOBAL");
    break;
  case clang::LangAS::opencl_local:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("LOCAL");
    break;
  case clang::LangAS::opencl_constant:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("CONSTANT");
    break;
  case clang::LangAS::opencl_generic:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("GENERIC");
    break;
  case clang::LangAS::opencl_private:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("PRIVATE");
    break;
  case clang::LangAS::cuda_constant:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("CUDA_CONSTANT");
    break;
  case clang::LangAS::cuda_device:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("CUDA_DEVICE");
    break;
  case clang::LangAS::cuda_shared:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("CUDA_SHARED");
    break;
  default:
    out->address_space_qualifier =
        enumValue<obj::AddressSpaceQualifierV2>("DEFAULT");
    break;
  }
  out->address_space = isTargetAddressSpace(addressSpace)
                          ? toTargetAddressSpace(addressSpace)
                          : 0;
  out->unqualified_type = wireId(clava::getId(type.getTypePtr(), c.id));
  return out;
}

std::unique_ptr<obj::BuiltinTypeDataT> makeBuiltinTypeData(
    const clang::BuiltinType *type, Context &c) {
  auto out = typeBase<obj::BuiltinTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->kind = enumValue<obj::BuiltinKind>(clava::BUILTIN_KIND[type->getKind()]);
  out->kind_literal = type->getName(c.ast->getPrintingPolicy()).str();
  return out;
}

std::unique_ptr<obj::PointerTypeDataT> makePointerTypeData(
    const clang::PointerType *type, Context &c) {
  auto out = typeBase<obj::PointerTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->pointee_type = wireId(clava::getId(type->getPointeeType(), c.id));
  return out;
}

std::unique_ptr<obj::FunctionTypeDataT> makeFunctionTypeData(
    const clang::FunctionType *type, Context &c) {
  auto out = typeBase<obj::FunctionTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->is_const = type->isConst();
  out->is_volatile = type->isVolatile();
  out->is_restrict = type->isRestrict();
  auto extInfo = type->getExtInfo();
  out->no_return = extInfo.getNoReturn();
  out->produces_result = extInfo.getProducesResult();
  out->uses_reg_parm = extInfo.getHasRegParm();
  out->reg_parm = extInfo.getHasRegParm() ? extInfo.getRegParm() : 0;
  out->calling_convention =
      enumValue<obj::CallingConvention>(clava::CALLING_CONVENTION[extInfo.getCC()]);
  out->return_type = wireId(clava::getId(type->getReturnType(), c.id));
  return out;
}

std::unique_ptr<obj::FunctionProtoTypeDataT> makeFunctionProtoTypeData(
    const clang::FunctionProtoType *type, Context &c) {
  auto out = typeBase<obj::FunctionProtoTypeDataT>(type);
  out->base = makeFunctionTypeData(type, c);
  out->num_parameters = static_cast<int>(type->getParamTypes().size());
  for (clang::QualType parameter : type->getParamTypes())
    out->parameters_types.push_back(wireId(clava::getId(parameter, c.id)));
  auto info = type->getExtProtoInfo();
  out->has_trailing_returns = static_cast<bool>(info.HasTrailingReturn);
  out->is_variadic = static_cast<bool>(info.Variadic);
  out->reference_qualifier =
      enumValue<obj::ReferenceQualifier>(clava::REFERENCE_QUALIFIER[info.RefQualifier]);
  out->exception_specification = makeExceptionSpecification(type, c);
  return out;
}

std::unique_ptr<obj::TagTypeDataT> makeTagTypeData(const clang::TagType *type,
                                                  Context &c) {
  auto out = typeBase<obj::TagTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->decl = wireId(clava::getId(type->getDecl(), c.id));
  return out;
}

std::unique_ptr<obj::ArrayTypeDataT> makeArrayTypeData(
    const clang::ArrayType *type, Context &c) {
  auto out = typeBase<obj::ArrayTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->array_size_modifier = enumValue<obj::ArraySizeModifier>(
      clava::ARRAY_SIZE_MODIFIER[llvm::to_underlying(type->getSizeModifier())]);
  out->index_type_qualifiers = c99Qualifiers(type->getIndexTypeQualifiers(), c);
  out->element_type = wireId(clava::getId(type->getElementType(), c.id));
  return out;
}

std::unique_ptr<obj::ConstantArrayTypeDataT> makeConstantArrayTypeData(
    const clang::ConstantArrayType *type, Context &c) {
  auto out = typeBase<obj::ConstantArrayTypeDataT>(type);
  out->base = makeArrayTypeData(type, c);
  llvm::SmallString<32> size;
  type->getSize().toString(size, 10, false);
  out->array_size = size.str().str();
  return out;
}

std::unique_ptr<obj::VariableArrayTypeDataT> makeVariableArrayTypeData(
    const clang::VariableArrayType *type, Context &c) {
  auto out = typeBase<obj::VariableArrayTypeDataT>(type);
  out->base = makeArrayTypeData(type, c);
  out->size_expr = wireId(clava::getId(type->getSizeExpr(), c.id));
  return out;
}

std::unique_ptr<obj::DependentSizedArrayTypeDataT>
makeDependentSizedArrayTypeData(const clang::DependentSizedArrayType *type,
                                Context &c) {
  auto out = typeBase<obj::DependentSizedArrayTypeDataT>(type);
  out->base = makeArrayTypeData(type, c);
  out->size_expr = wireId(clava::getId(type->getSizeExpr(), c.id));
  return out;
}

std::unique_ptr<obj::TypeWithKeywordDataT> makeTypeWithKeywordData(
    const clang::TypeWithKeyword *type, Context &c) {
  auto out = typeBase<obj::TypeWithKeywordDataT>(type);
  out->base = makeTypeData(type, c);
  out->elaborated_type_keyword = enumValue<obj::ElaboratedTypeKeyword>(
      clava::ELABORATED_TYPE_KEYWORD[llvm::to_underlying(type->getKeyword())]);
  return out;
}

std::unique_ptr<obj::ElaboratedTypeDataT> makeElaboratedTypeData(
    const clang::ElaboratedType *type, Context &c) {
  auto out = typeBase<obj::ElaboratedTypeDataT>(type);
  out->base = makeTypeWithKeywordData(type, c);
  out->qualifier = qualifierString(type->getQualifier(), c);
  out->named_type = wireId(clava::getId(type->getNamedType(), c.id));
  return out;
}

std::unique_ptr<obj::TemplateTypeParmTypeDataT> makeTemplateTypeParmTypeData(
    const clang::TemplateTypeParmType *type, Context &c) {
  auto out = typeBase<obj::TemplateTypeParmTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->depth = static_cast<int>(type->getDepth());
  out->index = static_cast<int>(type->getIndex());
  out->is_packed = type->isParameterPack();
  out->decl = wireId(clava::getId(type->getDecl(), c.id));
  return out;
}

std::unique_ptr<obj::TemplateSpecializationTypeDataT>
makeTemplateSpecializationTypeData(
    const clang::TemplateSpecializationType *type, Context &c) {
  auto out = typeBase<obj::TemplateSpecializationTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->is_type_alias = type->isTypeAlias();
  out->aliased_type = type->isTypeAlias()?wireId(clava::getId(type->getAliasedType(), c.id)):-1;
  out->template_name = templateNameText(type->getTemplateName());
  out->template_decl = wireId(
      clava::getId(type->getTemplateName().getAsTemplateDecl(), c.id));
  for (const auto &arg : type->template_arguments())
    out->template_arguments.push_back(makeTemplateArgument(arg, c));
  return out;
}

std::unique_ptr<obj::TypedefTypeDataT> makeTypedefTypeData(
    const clang::TypedefType *type, Context &c) {
  auto out = typeBase<obj::TypedefTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->decl = wireId(clava::getId(type->getDecl(), c.id));
  return out;
}

std::unique_ptr<obj::AdjustedTypeDataT> makeAdjustedTypeData(
    const clang::AdjustedType *type, Context &c) {
  auto out = typeBase<obj::AdjustedTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->original_type = wireId(clava::getId(type->getOriginalType(), c.id));
  out->adjusted_type = wireId(clava::getId(type->getAdjustedType(), c.id));
  return out;
}

std::unique_ptr<obj::DecayedTypeDataT> makeDecayedTypeData(
    const clang::DecayedType *type, Context &c) {
  auto out = typeBase<obj::DecayedTypeDataT>(type);
  out->base = makeAdjustedTypeData(type, c);
  out->decayed_type = wireId(clava::getId(type->getDecayedType(), c.id));
  out->pointee_type = wireId(clava::getId(type->getPointeeType(), c.id));
  return out;
}

std::unique_ptr<obj::DecltypeTypeDataT> makeDecltypeTypeData(
    const clang::DecltypeType *type, Context &c) {
  auto out = typeBase<obj::DecltypeTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->is_sugared = type->isSugared();
  out->underlying_expr = wireId(clava::getId(type->getUnderlyingExpr(), c.id));
  return out;
}

std::unique_ptr<obj::AutoTypeDataT> makeAutoTypeData(const clang::AutoType *type,
                                                    Context &c) {
  auto out = typeBase<obj::AutoTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->deduced_type = wireId(clava::getId(type->getDeducedType(), c.id));
  return out;
}

std::unique_ptr<obj::ReferenceTypeDataT> makeReferenceTypeData(
    const clang::ReferenceType *type, Context &c) {
  auto out = typeBase<obj::ReferenceTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->pointee_type_as_written =
      wireId(clava::getId(type->getPointeeTypeAsWritten(), c.id));
  return out;
}

std::unique_ptr<obj::PackExpansionTypeDataT> makePackExpansionTypeData(
    const clang::PackExpansionType *type, Context &c) {
  auto out = typeBase<obj::PackExpansionTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->num_expansions = type->getNumExpansions().value_or(0);
  out->pattern = wireId(clava::getId(type->getPattern(), c.id));
  return out;
}

std::unique_ptr<obj::TypeOfExprTypeDataT> makeTypeOfExprTypeData(
    const clang::TypeOfExprType *type, Context &c) {
  auto out = typeBase<obj::TypeOfExprTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->is_sugared = type->isSugared();
  out->underlying_expr = wireId(clava::getId(type->getUnderlyingExpr(), c.id));
  return out;
}

std::unique_ptr<obj::AttributedTypeDataT> makeAttributedTypeData(
    const clang::AttributedType *type, Context &c) {
  auto out = typeBase<obj::AttributedTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->modified_type = wireId(clava::getId(type->getModifiedType(), c.id));
  out->equivalent_type = wireId(clava::getId(type->getEquivalentType(), c.id));
  return out;
}

std::unique_ptr<obj::UnaryTransformTypeDataT> makeUnaryTransformTypeData(
    const clang::UnaryTransformType *type, Context &c) {
  auto out = typeBase<obj::UnaryTransformTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->kind = enumValue<obj::UnaryTransformTypeKind>(
      clava::UTT_KIND[llvm::to_underlying(type->getUTTKind())]);
  out->underlying_type = wireId(clava::getId(type->getUnderlyingType(), c.id));
  out->base_type = wireId(clava::getId(type->getBaseType(), c.id));
  return out;
}

std::unique_ptr<obj::SubstTemplateTypeParmTypeDataT>
makeSubstTemplateTypeParmTypeData(
    const clang::SubstTemplateTypeParmType *type, Context &c) {
  auto out = typeBase<obj::SubstTemplateTypeParmTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->replaced_parameter =
      wireId(clava::getId(type->getReplacedParameter(), c.id));
  out->replacement_type = wireId(clava::getId(type->getReplacementType(), c.id));
  return out;
}

std::unique_ptr<obj::ComplexTypeDataT> makeComplexTypeData(
    const clang::ComplexType *type, Context &c) {
  auto out = typeBase<obj::ComplexTypeDataT>(type);
  out->base = makeTypeData(type, c);
  out->element_type = wireId(clava::getId(type->getElementType(), c.id));
  return out;
}

std::unique_ptr<obj::FunctionTypeDataT> makeFunctionNoProtoTypeData(
    const clang::FunctionNoProtoType *type, Context &c) {
  return makeFunctionTypeData(type, c);
}
std::unique_ptr<obj::ArrayTypeDataT> makeIncompleteArrayTypeData(
    const clang::IncompleteArrayType *type, Context &c) {
  return makeArrayTypeData(type, c);
}
std::unique_ptr<obj::TagTypeDataT> makeRecordTypeData(
    const clang::RecordType *type, Context &c) {
  return makeTagTypeData(type, c);
}
std::unique_ptr<obj::TagTypeDataT> makeEnumTypeData(const clang::EnumType *type,
                                                   Context &c) {
  return makeTagTypeData(type, c);
}
std::unique_ptr<obj::ReferenceTypeDataT> makeLValueReferenceTypeData(
    const clang::LValueReferenceType *type, Context &c) {
  return makeReferenceTypeData(type, c);
}
std::unique_ptr<obj::ReferenceTypeDataT> makeRValueReferenceTypeData(
    const clang::RValueReferenceType *type, Context &c) {
  return makeReferenceTypeData(type, c);
}

} // namespace clava::proto
