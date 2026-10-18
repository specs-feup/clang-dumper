#include "FlatSupport.h"

#include "../Clang/ClangNodes.h"
#include "../ClangEnums/ClangEnums.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/STLForwardCompat.h"

namespace clava::flat {

namespace {

std::string templateNameText(const clang::TemplateName &name) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  name.dump(stream);
  return text;
}

std::string unsignedDecimal(const llvm::APInt &value) {
  llvm::SmallString<32> text;
  value.toString(text, 10, false);
  return text.str().str();
}

} // namespace

std::unique_ptr<fb::TypeDataT> makeTypeData(const clang::Type *type,
                                            Context &c,
                                            clang::Qualifiers qualifiers) {
  auto out = std::make_unique<fb::TypeDataT>();
  out->type_as_string =
      clang::QualType::getAsString(type, qualifiers, c.ast->getPrintingPolicy());
  if (type->isDependentType())
    out->type_dependency = enumValue<fb::TypeDependency>("DEPENDENT");
  else if (type->isInstantiationDependentType())
    out->type_dependency =
        enumValue<fb::TypeDependency>("INSTANTIATION_DEPENDENT");
  else
    out->type_dependency = enumValue<fb::TypeDependency>("NONE");
  out->is_variably_modified = type->isVariablyModifiedType();
  out->contains_unexpanded_parameter_pack =
      type->containsUnexpandedParameterPack();
  out->is_from_ast = type->isFromAST();
  clang::QualType desugared = type->getLocallyUnqualifiedSingleStepDesugaredType();
  out->unqualified_desugared_type = -1;
  if (desugared != clang::QualType(type, 0))
    out->unqualified_desugared_type =
        clava::getId(desugared, c.id);
  return out;
}

std::unique_ptr<fb::TypeDataT> makeTypeData(const clang::Type *type,
                                            Context &c) {
  return makeTypeData(type, c, clang::Qualifiers());
}

std::unique_ptr<fb::QualTypeDataT> makeQualTypeData(const clang::QualType &type,
                                                    Context &c) {
  auto out = std::make_unique<fb::QualTypeDataT>();
  auto qualifiers = type.getQualifiers();
  out->base = makeTypeData(type.getTypePtr(), c, qualifiers);
  out->c99_qualifiers = c99Qualifiers(qualifiers, c);

  clang::LangAS addressSpace = type.getAddressSpace();
  switch (addressSpace) {
  case clang::LangAS::Default:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("NONE");
    break;
  case clang::LangAS::opencl_global:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("GLOBAL");
    break;
  case clang::LangAS::opencl_local:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("LOCAL");
    break;
  case clang::LangAS::opencl_constant:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("CONSTANT");
    break;
  case clang::LangAS::opencl_generic:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("GENERIC");
    break;
  case clang::LangAS::opencl_private:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("PRIVATE");
    break;
  case clang::LangAS::cuda_constant:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("CUDA_CONSTANT");
    break;
  case clang::LangAS::cuda_device:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("CUDA_DEVICE");
    break;
  case clang::LangAS::cuda_shared:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("CUDA_SHARED");
    break;
  default:
    out->address_space_qualifier =
        enumValue<fb::AddressSpaceQualifierV2>("DEFAULT");
    break;
  }
  out->address_space = isTargetAddressSpace(addressSpace)
                          ? toTargetAddressSpace(addressSpace)
                          : 0;
  out->unqualified_type = clava::getId(type.getTypePtr(), c.id);
  return out;
}

// The builders of the other type payloads, generated from wire/v2/types.fbs.
#include "types_builders.inc"

fb::NodeT makeNode(const clang::QualType &node, Context &c) {
  fb::NodeT out;
  out.id = clava::getId(node, c.id);
  out.class_name = "QualType";
  out.payload.Set(std::move(*makeQualTypeData(node, c)));
  return out;
}

} // namespace clava::flat
