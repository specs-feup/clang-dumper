#include "FlatSupport.h"

namespace clava::flat {

std::unique_ptr<fb::AttributeDataT> makeAttributeData(const clang::Attr *node, Context &c) {
  auto out = std::make_unique<fb::AttributeDataT>();
  out->base = makeNodeData(node->getRange().getBegin(), node->getRange().getEnd(), c);
  out->kind = enumValue<fb::AttributeKind>(clava::getAttrKind(node));
  out->is_implicit = node->isImplicit();
  out->is_inherited = node->isInherited();
  out->is_late_parsed = node->isLateParsed();
  out->is_pack_expansion = node->isPackExpansion();
  return out;
}

static std::vector<int32_t> nonNullArguments(const clang::NonNullAttr *node) {
  std::vector<int32_t> out;
  for (auto argument : node->args())
    out.push_back(argument.getSourceIndex());
  return out;
}

#include "attributes_builders.inc"

} // namespace clava::flat
