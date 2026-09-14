#include "ProtoSupport.h"
#include "llvm/ADT/SmallString.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include <charconv>
#include <stdexcept>

namespace clava::proto {
int64_t wireId(const std::string &value) {
  if (value.starts_with("@")) {
    int64_t id=0; auto r=std::from_chars(value.data()+1,value.data()+value.size(),id);
    if(r.ec==std::errc() && r.ptr==value.data()+value.size() && id>0)return id;
  }
  if(value=="nullptr_type")return -1;
  if(value=="nullptr_decl")return -2;
  if(value=="nullptr_expr")return -3;
  if(value=="nullptr_stmt")return -4;
  if(value=="nullptr_attr")return -5;
  if(value.starts_with("0_"))return -6;
  throw std::invalid_argument("Not a dense node reference: "+value);
}
static clang::SourceLocation unsplit(const clang::SourceManager &sm, clang::SourceLocation loc) {
  while(loc.isMacroID()) {
    auto range=sm.getImmediateExpansionRange(loc);
    if(range.isTokenRange())break;
    auto next=range.getBegin().getLocWithOffset(sm.getDecomposedLoc(loc).second);
    if(next.isInvalid()||next==loc)break;
    loc=next;
  }
  return loc;
}
static std::unique_ptr<obj::RangeT> range(clang::SourceLocation begin, clang::SourceLocation end, Context &c) {
  auto &sm=c.ast->getSourceManager(); auto b=sm.getSpellingLoc(begin);
  if(b.isInvalid())return nullptr;
  auto out=std::make_unique<obj::RangeT>();
  auto path=[&](clang::SourceLocation p){auto name=sm.getFilename(p);return name.empty()?sm.getBufferName(p):name;};
  out->file=c.fileId(path(b));out->line=sm.getSpellingLineNumber(b);out->column=sm.getSpellingColumnNumber(b);
  out->end_file=0;out->end_line=0;out->end_column=0;
  auto e=sm.getSpellingLoc(end);
  if(begin!=end && e.isValid()) {
    out->end_file=c.fileId(path(e));out->end_line=sm.getSpellingLineNumber(e);out->end_column=sm.getSpellingColumnNumber(e);
  }
  return out;
}
std::unique_ptr<obj::NodeDataT> makeNodeData(clang::SourceLocation begin, clang::SourceLocation end, Context &c) {
  auto out=std::make_unique<obj::NodeDataT>();out->source=std::make_unique<obj::SourceInfoT>();
  auto &s=*out->source;auto &sm=c.ast->getSourceManager();begin=unsplit(sm,begin);end=unsplit(sm,end);
  auto expanded=sm.getExpansionRange(clang::SourceRange(begin,end));
  s.expansion=range(expanded.getBegin(),expanded.getEnd(),c);
  bool macro=begin.isMacroID()||end.isMacroID();s.is_macro=macro;
  if(macro)s.spelling=range(sm.getSpellingLoc(begin),sm.getSpellingLoc(end),c);
  auto full=c.ast->getFullLoc(begin);s.system_header=full.isValid()&&full.isInSystemHeader();
  return out;
}
std::string qualifierString(clang::NestedNameSpecifier *q, Context &c) {
  std::string s;if(q){llvm::raw_string_ostream stream(s);q->print(stream,c.ast->getPrintingPolicy());}return s;
}
std::string sourceText(clang::SourceRange range, Context &c) {
  const auto &sm = c.ast->getSourceManager();
  auto begin = range.getBegin();
  auto end = range.getEnd();
  if (begin.isMacroID())
    begin = sm.getSpellingLoc(begin);
  if (end.isMacroID())
    end = sm.getSpellingLoc(end);
  auto text = clang::Lexer::getSourceText(
      clang::CharSourceRange::getTokenRange(begin, end), sm,
      clang::LangOptions(), 0).str();
  if (!text.empty() && text.back() == ',')
    return clang::Lexer::getSourceText(
               clang::CharSourceRange::getCharRange(begin, end), sm,
               clang::LangOptions(), 0)
        .str();
  return text;
}
std::vector<obj::C99Qualifier> c99Qualifiers(clang::Qualifiers q, Context &c) {
  std::vector<obj::C99Qualifier> out;
  if(q.hasConst())out.push_back(obj::C99Qualifier::CONST);
  if(q.hasRestrict())out.push_back(c.ast->getPrintingPolicy().Restrict?obj::C99Qualifier::RESTRICT_C99:obj::C99Qualifier::RESTRICT);
  if(q.hasVolatile())out.push_back(obj::C99Qualifier::VOLATILE);
  return out;
}
std::unique_ptr<obj::TemplateNameT> makeTemplateName(const clang::TemplateName &n, Context &c) {
  auto out=std::make_unique<obj::TemplateNameT>();
  switch(n.getKind()) {
  case clang::TemplateName::Template: {obj::DirectTemplateNameT v;v.template_decl=wireId(clava::getId(n.getAsTemplateDecl(),c.id));out->value.Set(std::move(v));break;}
  case clang::TemplateName::QualifiedTemplate: {obj::QualifiedTemplateNameT v;auto q=n.getAsQualifiedTemplateName();v.qualifier=qualifierString(q->getQualifier(),c);v.has_template_keyword=q->hasTemplateKeyword();v.template_decl=wireId(clava::getId(n.getAsTemplateDecl(),c.id));out->value.Set(std::move(v));break;}
  case clang::TemplateName::SubstTemplateTemplateParm: {obj::SubstitutedTemplateNameT v;auto p=n.getAsSubstTemplateTemplateParm();v.parameter=wireId(clava::getId(p->getParameter(),c.id));v.replacement=makeTemplateName(p->getReplacement(),c);out->value.Set(std::move(v));break;}
  case clang::TemplateName::UsingTemplate: {obj::UsingTemplateNameT v;v.using_shadow_decl=wireId(clava::getId(n.getAsUsingShadowDecl(),c.id));out->value.Set(std::move(v));break;}
  case clang::TemplateName::DependentTemplate: {obj::DependentTemplateNameT v;auto p=n.getAsDependentTemplateName();v.qualifier=qualifierString(p->getQualifier(),c);v.name=p->isIdentifier()?p->getIdentifier()->getName().str():clang::getOperatorSpelling(p->getOperator());out->value.Set(std::move(v));break;}
  default:throw std::invalid_argument("Unsupported template name kind");
  }
  return out;
}
std::unique_ptr<obj::TemplateArgumentT> makeTemplateArgument(const clang::TemplateArgument &a, Context &c) {
  auto out=std::make_unique<obj::TemplateArgumentT>();
  switch(a.getKind()) {
  case clang::TemplateArgument::Declaration:{obj::TemplateDeclarationT v;v.decl=wireId(clava::getId(a.getAsDecl(),c.id));out->value.Set(std::move(v));break;}
  case clang::TemplateArgument::NullPtr:{obj::TemplateNullPtrT v;v.type=wireId(clava::getId(a.getNullPtrType(),c.id));out->value.Set(std::move(v));break;}
  case clang::TemplateArgument::Type:{obj::TemplateTypeT v;v.type=wireId(clava::getId(a.getAsType(),c.id));out->value.Set(std::move(v));break;}
  case clang::TemplateArgument::Expression:{obj::TemplateExpressionT v;v.expr=wireId(clava::getId(a.getAsExpr(),c.id));out->value.Set(std::move(v));break;}
  case clang::TemplateArgument::Pack:{obj::TemplatePackT v;for(auto &p:a.pack_elements())v.arguments.push_back(makeTemplateArgument(p,c));out->value.Set(std::move(v));break;}
  case clang::TemplateArgument::Integral:{obj::TemplateIntegralT v;llvm::SmallString<32> s;a.getAsIntegral().toString(s,10);v.integral=s.str().str();out->value.Set(std::move(v));break;}
  case clang::TemplateArgument::Template:out->value.Set(std::move(*makeTemplateName(a.getAsTemplate(),c)));break;
  case clang::TemplateArgument::TemplateExpansion:{obj::TemplateExpansionT v;if(auto n=a.getNumTemplateExpansions())v.num_expansions=*n;v.template_name=makeTemplateName(a.getAsTemplateOrTemplatePattern(),c);out->value.Set(std::move(v));break;}
  case clang::TemplateArgument::StructuralValue:{obj::TemplateStructuralValueT v;v.type=wireId(clava::getId(a.getStructuralValueType(),c.id));out->value.Set(std::move(v));break;}
  default:throw std::invalid_argument("Unsupported template argument kind");
  }
  return out;
}
std::unique_ptr<obj::CXXBaseSpecifierT> makeCXXBaseSpecifier(const clang::CXXBaseSpecifier &b, Context &c) {
  auto v=std::make_unique<obj::CXXBaseSpecifierT>();v->is_virtual=b.isVirtual();v->is_pack_expansion=b.isPackExpansion();
  v->access_specifier_as_written=enumValue<obj::AccessSpecifier>(clava::ACCESS_SPECIFIER[b.getAccessSpecifierAsWritten()]);
  v->access_specifier_semantic=enumValue<obj::AccessSpecifier>(clava::ACCESS_SPECIFIER[b.getAccessSpecifier()]);v->type=wireId(clava::getId(b.getType(),c.id));return v;
}
std::unique_ptr<obj::ExplicitSpecifierT> makeExplicitSpecifier(const clang::ExplicitSpecifier &s, Context &c) {
  auto v=std::make_unique<obj::ExplicitSpecifierT>();v->kind=enumValue<obj::ExplicitSpecKind>(clava::EXPLICIT_SPEC_KIND[static_cast<unsigned>(s.getKind())]);v->expr=wireId(clava::getId(s.getExpr(),c.id));v->is_specified=s.isSpecified();return v;
}
std::unique_ptr<obj::CXXCtorInitializerT> makeCXXCtorInitializer(const clang::CXXCtorInitializer *i, Context &c) {
  auto v=std::make_unique<obj::CXXCtorInitializerT>();
  if(i->isAnyMemberInitializer()){obj::AnyMemberInitializerT x;x.any_member_decl=wireId(clava::getId(i->getAnyMember(),c.id));v->target.Set(std::move(x));}
  else if(i->isBaseInitializer()){obj::BaseInitializerT x;x.base_class=wireId(clava::getId(i->getBaseClass(),c.id));v->target.Set(std::move(x));}
  else if(i->isDelegatingInitializer()){obj::DelegatingInitializerT x;x.delegated_type=wireId(clava::getId(i->getTypeSourceInfo()->getType(),c.id));v->target.Set(std::move(x));}
  else throw std::invalid_argument("Unsupported constructor initializer");
  v->init_expr=wireId(clava::getId(i->getInit(),c.id));v->is_in_class_member_initializer=i->isInClassMemberInitializer();v->is_written=i->isWritten();return v;
}
std::unique_ptr<obj::ExceptionSpecificationT> makeExceptionSpecification(const clang::FunctionProtoType *t, Context &c) {
  auto v=std::make_unique<obj::ExceptionSpecificationT>();auto s=t->getExtProtoInfo().ExceptionSpec;
  v->kind=enumValue<obj::ExceptionSpecificationType>(clava::EXCEPTION_SPECIFICATION_TYPE[s.Type]);
  for(auto q:s.Exceptions)v->exception_types.push_back(wireId(clava::getId(q,c.id)));
  switch(s.Type){
  case clang::EST_DependentNoexcept:{obj::ComputedExceptionDetailsT x;x.noexcept_expr=wireId(clava::getId(s.NoexceptExpr,c.id));v->details.Set(std::move(x));break;}
  case clang::EST_Unevaluated:{obj::UnevaluatedExceptionDetailsT x;x.source_decl=wireId(clava::getId(s.SourceDecl,c.id));v->details.Set(std::move(x));break;}
  case clang::EST_Uninstantiated:{obj::UninstantiatedExceptionDetailsT x;x.source_decl=wireId(clava::getId(s.SourceDecl,c.id));x.source_template=wireId(clava::getId(s.SourceTemplate,c.id));v->details.Set(std::move(x));break;}
  default:v->details.Set(obj::NoExceptionDetailsT{});
  }
  return v;
}
std::unique_ptr<obj::OffsetOfComponentT> makeOffsetOfComponent(const clang::OffsetOfExpr *e,unsigned i, Context &c) {
  auto v=std::make_unique<obj::OffsetOfComponentT>();auto n=e->getComponent(i);
  switch(n.getKind()){
  case clang::OffsetOfNode::Array:{obj::OffsetArrayT x;x.expr=wireId(clava::getId(e->getIndexExpr(n.getArrayExprIndex()),c.id));v->value.Set(std::move(x));break;}
  case clang::OffsetOfNode::Base:{obj::OffsetBaseT x;x.type=wireId(clava::getId(n.getBase()->getType(),c.id));v->value.Set(std::move(x));break;}
  case clang::OffsetOfNode::Field:{obj::OffsetFieldT x;x.field_name=n.getFieldName()->getName().str();v->value.Set(std::move(x));break;}
  case clang::OffsetOfNode::Identifier:{obj::OffsetIdentifierT x;x.field_name=n.getFieldName()->getName().str();v->value.Set(std::move(x));break;}
  }
  return v;
}
std::unique_ptr<obj::DesignatorT> makeDesignator(const clang::DesignatedInitExpr::Designator *d,Context &c) {
  auto v=std::make_unique<obj::DesignatorT>();
  if(d->isFieldDesignator()){obj::FieldDesignatorT x;x.field_name=d->getFieldName()?d->getFieldName()->getName().str():"";v->value.Set(std::move(x));}
  else if(d->isArrayDesignator()){obj::ArrayDesignatorT x;x.index=d->getArrayIndex();v->value.Set(std::move(x));}
  else if(d->isArrayRangeDesignator()){obj::ArrayRangeDesignatorT x;x.index=d->getArrayIndex();v->value.Set(std::move(x));}
  else throw std::invalid_argument("Unsupported designator");
  return v;
}
std::unique_ptr<obj::NestedNameSpecifierT> makeNestedNameSpecifier(clang::NestedNameSpecifier *n, Context &c) {
  if (n == nullptr)
    return nullptr;
  auto v=std::make_unique<obj::NestedNameSpecifierT>();
  switch(n->getKind()){
  case clang::NestedNameSpecifier::Namespace:{obj::NamespaceSpecifierT x;x.namespace_decl=wireId(clava::getId(n->getAsNamespace(),c.id));v->value.Set(std::move(x));break;}
  case clang::NestedNameSpecifier::NamespaceAlias:{obj::NamespaceAliasSpecifierT x;x.namespace_alias=wireId(clava::getId(n->getAsNamespaceAlias(),c.id));v->value.Set(std::move(x));break;}
  case clang::NestedNameSpecifier::TypeSpec:{obj::TypeSpecifierT x;x.type=wireId(clava::getId(n->getAsType(),c.id));v->value.Set(std::move(x));break;}
  case clang::NestedNameSpecifier::TypeSpecWithTemplate:{obj::TypeWithTemplateSpecifierT x;x.type=wireId(clava::getId(n->getAsType(),c.id));v->value.Set(std::move(x));break;}
  case clang::NestedNameSpecifier::Global:v->value.Set(obj::GlobalSpecifierT{});break;
  case clang::NestedNameSpecifier::Super:{obj::SuperSpecifierT x;x.super_decl=wireId(clava::getId(n->getAsRecordDecl(),c.id));v->value.Set(std::move(x));break;}
  default:throw std::invalid_argument("Unsupported nested name specifier");
  }
  return v;
}
}
