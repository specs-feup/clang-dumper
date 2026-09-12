#!/usr/bin/env python3
"""Generate native/Java bindings from FlatBuffers' own reflection schema."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess,tempfile,shutil
P=Path(__file__).resolve().parents[1]
def attrs(field):return {a['key']:a.get('value','') for a in field.get('attributes',[])}
def short(name):return name.rsplit('.',1)[-1]
def camel(name):
 parts=name.split('_');return parts[0]+''.join(p[:1].upper()+p[1:] for p in parts[1:])
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--flatc',required=True);ap.add_argument('--sdk',type=Path,required=True);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--java-out',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=True)
 destination=a.out
 temporary=tempfile.TemporaryDirectory(prefix='clava-wire-')
 a.out=Path(temporary.name)
 schemas=sorted((P/'wire/v2').glob('*.fbs'));schema=P/'wire/v2/complete.fbs'
 subprocess.run([a.flatc,'--cpp','--scoped-enums','--gen-object-api','-o',str(a.out),*map(str,schemas)],check=True)
 subprocess.run([a.flatc,'-b','--schema','-o',str(a.out),str(schema)],check=True)
 subprocess.run([a.flatc,'--json','--strict-json','-o',str(a.out),str(a.sdk/'reflection/reflection.fbs'),'--',str(a.out/'complete.bfbs')],check=True)
 spec=json.loads((a.out/'complete.json').read_text())
 digest=hashlib.sha256()
 for p in schemas:digest.update(p.name.encode()+b'\0'+p.read_bytes()+b'\0')
 hash=digest.hexdigest();(a.out/'FlatSchemaHash.h').write_text('#pragma once\nnamespace clava::flat { inline constexpr const char *SchemaHash = "'+hash+'"; }\n')
 enums=spec['enums'];objects=spec['objects']
 e=['#pragma once','#include "enums_generated.h"','#include <string>','#include <string_view>','#include <stdexcept>','namespace clava::flat {','template<class E> struct EnumNames;']
 for en in enums:
  if en.get('is_union'):continue
  n=en['name'].replace('.', '::');sn=short(en['name']);e.append('template<> struct EnumNames<'+n+'> { static const auto &values() { return astwire::v2::EnumValues'+sn+'(); } static const char *name('+n+' v) { return astwire::v2::EnumName'+sn+'(v); } };')
 e+=['inline std::string enumToken(std::string_view s) {std::string r; for(unsigned char c:s)if(c!=\'_\'&&c!=\'-\')r+=char(c>=\'A\'&&c<=\'Z\'?c+32:c);return r;}','template<class E> E enumValue(std::string_view name) { for(auto value:EnumNames<E>::values())if(name==EnumNames<E>::name(value))return value; auto token=enumToken(name); for(auto value:EnumNames<E>::values())if(token==enumToken(EnumNames<E>::name(value)))return value; throw std::invalid_argument("Unknown wire enum: "+std::string(name)); }','}']
 (a.out/'FlatEnumSupport.h').write_text('\n'.join(e)+'\n')
 decls=[]
 for f in sorted((P/'src/Clava').glob('Flat*.cpp')):
  for m in re.finditer(r'(std::unique_ptr<fb::\w+T>\s+make\w+\([^{};]+\))\s*\{',f.read_text()):decls.append(m[1]+';')
 (a.out/'FlatHandlerDeclarations.inc').write_text('\n'.join(sorted(set(decls)))+'\n')
 # Share the existing native dispatch inventory, including aliases/generic family handlers.
 dispatch=[]
 for family,ctype,source,default in [('DECL','Decl','Decls','Decl'),('STMT','Stmt','Stmts','Stmt'),('EXPR','Expr','Stmts','Expr'),('TYPE','Type','Types','Type'),('ATTR','Attr','Attrs','Attribute')]:
  body=(P/f'src/ClavaDataDumper/ClavaDataDumper{source}.cpp').read_text(); body=body[body.index(f'::{family}_DATA_DUMPERS ='):];body=body[:body.index('};')]
  entries=re.findall(f'{family}_DATA_ENTRY(_AS)?\\((\\w+)(?:,\\s*(\\w+))?\\)',body)
  dispatch.append(f'fb::NodeT makeNode(const clang::{ctype} *node, Context &c) {{ fb::NodeT out; out.id=wireId(clava::getId(node,c.id)); out.class_name=clava::getClassName(node);')
  for alias,cls,section in entries:
   section=section or cls
   dispatch.append(f'if(out.class_name=="{cls}"){{out.payload.Set(std::move(*make{section}Data(static_cast<const clang::{cls}*>(node),c)));return out;}}')
  dispatch.append(f'out.payload.Set(std::move(*make{default}Data(node,c)));return out;}}')
 dispatch.append('fb::NodeT makeNode(const clang::QualType &node, Context &c) {fb::NodeT out;out.id=wireId(clava::getId(node,c.id));out.class_name="QualType";out.payload.Set(std::move(*makeQualTypeData(node,c)));return out;}')
 (a.out/'FlatDispatch.inc').write_text('\n'.join(dispatch)+'\n')
 if a.java_out:
  a.java_out.mkdir(parents=True,exist_ok=True)
  subprocess.run([a.flatc,'--java','-o',str(a.java_out),*map(str,schemas)],check=True)
  generate_java(spec,hash,a.java_out)
 # Keep unchanged generated headers' timestamps so native getter edits do not
 # rebuild every translation unit that includes the schema.
 for source in a.out.iterdir():
  target=destination/source.name
  if not target.exists() or target.read_bytes()!=source.read_bytes():shutil.copyfile(source,target)
 (destination/'complete.stamp').touch()
 temporary.cleanup()

def generate_java(spec,hash,out):
 objects=spec['objects'];enums=spec['enums'];byname={short(o['name']):o for o in objects}
 def typename(t):return short(objects[t['index']]['name'])
 def flatten(o,path=''):
  fields=[]
  for f in sorted(o['fields'],key=lambda f:f.get('id',0)):
   if f['name']=='base':fields+=flatten(objects[f['type']['index']],path+'.base()')
   elif f['name']=='source' and short(o['name'])=='NodeData':fields.append(('source',f,path+'.source()'))
   elif attrs(f).get('java_key'):fields.append(('field',f,path+'.'+camel(f['name'])+'()'))
  return fields
 code=['package pt.up.fe.specs.clang.wire;','import com.google.flatbuffers.Table;','import java.util.*;','import org.suikasoft.jOptions.Datakey.DataKey;','import pt.up.fe.specs.clava.ClavaNode;','import static pt.up.fe.specs.clang.wire.SchemaRuntime.*;','public final class GeneratedNodes {',f' public static final String SCHEMA_HASH="{hash}";']
 nodeUnion=next(e for e in enums if short(e['name'])=='NodePayload')
 cases=[]
 for val in nodeUnion['values']:
  if val['name']=='NONE':continue
  n=val['name'];fields=flatten(byname[n]);bindings=[]
  for kind,f,path in fields:
   if kind=='source':
    bindings += [f'location(t -> ((astwire.v2.{n})t){path}.expansion())',f'scalar(ClavaNode.IS_MACRO,(t,p)->((astwire.v2.{n})t){path}.isMacro())',f'scalar(ClavaNode.IS_IN_SYSTEM_HEADER,(t,p)->((astwire.v2.{n})t){path}.systemHeader())'];continue
   at=attrs(f);key=at['java_key'];expr=f'((astwire.v2.{n})t){path}';typ=f['type'];bt=typ['base_type'];ref=at.get('java_ref');codec=at.get('java_codec','');en=at.get('java_enum')
   if ref:
    if at.get('java_ref_alias'):bindings.append(f'reference({at["java_ref_alias"]},"id",(t,c)->{expr})')
    if ref=='list':bindings.append(f'references({key},(t,c)->longs(((astwire.v2.{n})t){path[:-2]}Length(), i->((astwire.v2.{n})t){path[:-2]}(i)))')
    else:bindings.append(f'reference({key},"{ref}",(t,c)->{expr})')
    continue
   if not en and typ.get('index',-1)>=0 and bt not in ('Obj','Union') and (bt!='Vector' or typ.get('element')!='Obj'):en=attrs(enums[typ['index']]).get('java_enum')
   if codec in ('compound','compoundList','optionalCompound'):
    tt=typename(typ) if bt=='Obj' else typename({'index':typ['index']})
    x=f'CompoundReader.read{tt}({expr},c)'
    if codec=='optionalCompound':x=f'({expr}==null?Optional.empty():Optional.of({x}))'
    if bt=='Vector':x=f'list(((astwire.v2.{n})t){path[:-2]}Length(),i->CompoundReader.read{tt}(((astwire.v2.{n})t){path[:-2]}(i),c))'
    bindings.append(f'compound({key},(t,c)->{x})');continue
   if bt=='Vector':
    element=f'((astwire.v2.{n})t){path[:-2]}(i)'
    if en:element=f'{en}.values()[{element}]'
    elif typ['element']=='UByte':element=f'(byte){element}'
    expr=f'list(((astwire.v2.{n})t){path[:-2]}Length(),i->{element})'
   elif en:expr=f'{en}.values()[{expr}]'
   elif codec=='bigint':expr=f'new java.math.BigInteger({expr})'
   elif codec=='optionalString':expr=f'Optional.ofNullable({expr})'
   elif codec=='optionalEmptyString':expr=f'Optional.ofNullable({expr}).filter(s->!s.isEmpty())'
   elif codec=='optionalInt':expr=f'((astwire.v2.{n})t){path.rsplit(".",1)[0]}.has{camel(f["name"])[0].upper()+camel(f["name"])[1:]}()?Optional.of({expr}):Optional.empty()'
   elif codec and codec!='literalSource':raise ValueError((n,f['name'],'unknown codec',codec))
   bindings.append(f'scalar({key},(t,p)->{expr})')
  code.append(f' private static final Descriptor D_{n}=new Descriptor(List.of('+','.join(bindings)+'));')
  cases.append(f'case astwire.v2.NodePayload.{n}: return D_{n};')
 code.append('public static Descriptor descriptor(int kind){switch(kind){'+''.join(cases)+'default:throw new IllegalArgumentException("Unknown node payload "+kind);}}')
 cases=[]
 for val in nodeUnion['values']:
  if val['name']=='NONE':continue
  n=val['name'];cases.append(f'case astwire.v2.NodePayload.{n}:return node.payload(new astwire.v2.{n}());')
 code.append('public static Table payload(astwire.v2.Node node){switch(node.payloadType()){'+''.join(cases)+'default:throw new IllegalArgumentException("Unknown node payload");}}')
 # Generated presence/enum checks also traverse compound tables and union alternatives.
 for o in objects:
  n=short(o['name']);checks=[]
  for f in o['fields']:
   name=camel(f['name']);t=f['type'];bt=t['base_type'];at=attrs(f);get='v.'+name+'()'
   if bt=='UType':continue
   if bt=='Union':
    union=enums[t['index']];parts=[]
    for v in union['values']:
     if v['name']=='NONE':continue
     target=short(objects[v['union_type']['index']]['name']);parts.append(f'case astwire.v2.{short(union["name"])}.{v["name"]}:validate((astwire.v2.{target})v.{name}(new astwire.v2.{target}()));break;')
    checks.append(f'switch(v.{name}Type()){{'+''.join(parts)+f'default:throw new IllegalArgumentException("Missing/unknown {n}.{name}");}}');continue
   if bt=='Obj':checks.append(f'if({get}!=null)validate({get});'+(f'else missing("{n}.{name}");' if f.get('required') else ''));continue
   if bt=='String':
    if f.get('required'):checks.append(f'if(v.{name}AsByteBuffer()==null)missing("{n}.{name}");')
    continue
   if bt=='Vector':
    if f.get('required'):checks.append(f'if(v.{name}Vector()==null)missing("{n}.{name}");')
    if t['element']=='Obj':checks.append(f'for(int i=0;i<v.{name}Length();i++)validate(v.{name}(i));')
    elif t.get('index',-1)>=0:
     en=enums[t['index']];checks.append(f'for(int i=0;i<v.{name}Length();i++)if(v.{name}(i)<0||v.{name}(i)>={len(en["values"])})missing("Invalid enum {n}.{name}");')
    continue
   if f.get('optional') and 'wire_optional' not in at:checks.append(f'if(!v.has{name[0].upper()+name[1:]}())missing("{n}.{name}");')
   if t.get('index',-1)>=0:
    en=enums[t['index']];checks.append(f'if({get}<0||{get}>={len(en["values"])})missing("Invalid enum {n}.{name}");')
  code.append(f'public static void validate(astwire.v2.{n} v){{if(v==null)missing("{n}");'+''.join(checks)+'}')
 code.append('private static void missing(String field){throw new IllegalArgumentException("Missing or invalid wire field: "+field);}')
 code.append('}')
 dest=out/'pt/up/fe/specs/clang/wire';dest.mkdir(parents=True,exist_ok=True);(dest/'GeneratedNodes.java').write_text('\n'.join(code)+'\n')
if __name__=='__main__':main()
