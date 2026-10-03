#include "FlatSupport.h"
using namespace clang;
namespace clava::flat {
std::unique_ptr<fb::AttributeDataT> makeAttributeData(const Attr *a, Context &c) {
 auto out=std::make_unique<fb::AttributeDataT>();
 out->base=makeNodeData(a->getRange().getBegin(),a->getRange().getEnd(),c);
 out->kind=enumValue<fb::AttributeKind>(clava::getAttrKind(a));
 out->is_implicit=a->isImplicit(); out->is_inherited=a->isInherited();
 out->is_late_parsed=a->isLateParsed(); out->is_pack_expansion=a->isPackExpansion();return out;
}
std::unique_ptr<fb::AlignedAttrDataT> makeAlignedAttrData(const AlignedAttr *a, Context &c) {
 auto out=std::make_unique<fb::AlignedAttrDataT>();out->base=makeAttributeData(a,c);
 out->spelling=a->getSpelling();out->is_expression=a->isAlignmentExpr();
 out->alignment=a->isAlignmentExpr()?wireId(clava::getId(a->getAlignmentExpr(),c.id)):wireId(clava::getId(a->getAlignmentType()->getType(),c.id));return out;
}
std::unique_ptr<fb::OpenCLUnrollHintAttrDataT> makeOpenCLUnrollHintAttrData(const OpenCLUnrollHintAttr *a, Context &c) {
 auto out=std::make_unique<fb::OpenCLUnrollHintAttrDataT>();out->base=makeAttributeData(a,c);out->unroll_hint=a->getUnrollHint();return out;
}
std::unique_ptr<fb::FormatAttrDataT> makeFormatAttrData(const FormatAttr *a, Context &c) {
 auto out=std::make_unique<fb::FormatAttrDataT>();out->base=makeAttributeData(a,c);
 out->type=enumValue<fb::FormatAttrKind>(a->getType()->getName().str());out->format_index=a->getFormatIdx();out->first_arg=a->getFirstArg();return out;
}
std::unique_ptr<fb::NonNullAttrDataT> makeNonNullAttrData(const NonNullAttr *a, Context &c) {
 auto out=std::make_unique<fb::NonNullAttrDataT>();out->base=makeAttributeData(a,c);
 for(auto arg:a->args())out->arguments.push_back(arg.getSourceIndex());return out;
}
std::unique_ptr<fb::VisibilityAttrDataT> makeVisibilityAttrData(const VisibilityAttr *a, Context &c) {
 auto out=std::make_unique<fb::VisibilityAttrDataT>();out->base=makeAttributeData(a,c);
 out->visibility_type=enumValue<fb::VisibilityType>(clava::VISIBILITY_ATTR_TYPE[a->getVisibility()]);return out;
}
}
