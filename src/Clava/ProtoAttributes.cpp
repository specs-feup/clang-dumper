#include "ProtoSupport.h"
using namespace clang;
namespace clava::proto {
std::unique_ptr<obj::AttributeDataT> makeAttributeData(const Attr *a, Context &c) {
 auto out=std::make_unique<obj::AttributeDataT>();
 out->base=makeNodeData(a->getRange().getBegin(),a->getRange().getEnd(),c);
 out->kind=enumValue<obj::AttributeKind>(clava::getAttrKind(a));
 out->is_implicit=a->isImplicit(); out->is_inherited=a->isInherited();
 out->is_late_parsed=a->isLateParsed(); out->is_pack_expansion=a->isPackExpansion();return out;
}
std::unique_ptr<obj::AlignedAttrDataT> makeAlignedAttrData(const AlignedAttr *a, Context &c) {
 auto out=std::make_unique<obj::AlignedAttrDataT>();out->base=makeAttributeData(a,c);
 out->spelling=a->getSpelling();out->is_expression=a->isAlignmentExpr();
 out->alignment=a->isAlignmentExpr()?wireId(clava::getId(a->getAlignmentExpr(),c.id)):wireId(clava::getId(a->getAlignmentType()->getType(),c.id));return out;
}
std::unique_ptr<obj::OpenCLUnrollHintAttrDataT> makeOpenCLUnrollHintAttrData(const OpenCLUnrollHintAttr *a, Context &c) {
 auto out=std::make_unique<obj::OpenCLUnrollHintAttrDataT>();out->base=makeAttributeData(a,c);out->unroll_hint=a->getUnrollHint();return out;
}
std::unique_ptr<obj::FormatAttrDataT> makeFormatAttrData(const FormatAttr *a, Context &c) {
 auto out=std::make_unique<obj::FormatAttrDataT>();out->base=makeAttributeData(a,c);
 out->type=enumValue<obj::FormatAttrKind>(a->getType()->getName().str());out->format_index=a->getFormatIdx();out->first_arg=a->getFirstArg();return out;
}
std::unique_ptr<obj::NonNullAttrDataT> makeNonNullAttrData(const NonNullAttr *a, Context &c) {
 auto out=std::make_unique<obj::NonNullAttrDataT>();out->base=makeAttributeData(a,c);
 for(auto arg:a->args())out->arguments.push_back(arg.getSourceIndex());return out;
}
std::unique_ptr<obj::VisibilityAttrDataT> makeVisibilityAttrData(const VisibilityAttr *a, Context &c) {
 auto out=std::make_unique<obj::VisibilityAttrDataT>();out->base=makeAttributeData(a,c);
 out->visibility_type=enumValue<obj::VisibilityType>(clava::VISIBILITY_ATTR_TYPE[a->getVisibility()]);return out;
}
}
