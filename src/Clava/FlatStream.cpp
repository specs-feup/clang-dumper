#include "FlatStream.h"
#include "FlatSchemaHash.h"
namespace clava::flat {
namespace { FlatStream *activeStream=nullptr; }
FlatStream::FlatStream(llvm::raw_ostream &output):output(output) {
 if(activeStream)throw std::logic_error("Nested FlatBuffers dump streams");
 activeStream=this;fb::HeaderT header;header.schema_hash=SchemaHash;record(std::move(header));
}
FlatStream::~FlatStream(){if(activeStream==this)activeStream=nullptr;}
FlatStream &FlatStream::current() {
 if(!activeStream)throw std::logic_error("No AST dump in progress");
 return *activeStream;
}
void FlatStream::beginSourceFile() {
 inlineAsmLocations.clear();
 pendingAsmLocation={};
 trackingAsmQualifiers=false;
 pendingAsmInline=false;
}
void FlatStream::observePreprocessorToken(clang::tok::TokenKind kind,
                                         clang::SourceLocation location) {
 if(kind==clang::tok::kw_asm) {
   trackingAsmQualifiers=true;
   pendingAsmLocation=location;
   pendingAsmInline=false;
   return;
 }
 if(!trackingAsmQualifiers)return;
 if(kind==clang::tok::kw_inline)pendingAsmInline=true;
 if(kind==clang::tok::l_paren) {
   if(pendingAsmInline&&pendingAsmLocation.isValid())
     inlineAsmLocations.insert(pendingAsmLocation.getRawEncoding());
   trackingAsmQualifiers=false;
   pendingAsmLocation={};
   pendingAsmInline=false;
   return;
 }
 if(kind==clang::tok::semi||kind==clang::tok::l_brace||
    kind==clang::tok::r_brace||kind==clang::tok::eof) {
   trackingAsmQualifiers=false;
   pendingAsmLocation={};
   pendingAsmInline=false;
 }
}
bool FlatStream::isInlineAsm(clang::SourceLocation location) const {
 return location.isValid()&&inlineAsmLocations.contains(location.getRawEncoding());
}
uint32_t FlatStream::fileId(llvm::StringRef path) {
 auto [it,inserted]=files.try_emplace(path.str(),files.size()+1);
 if(inserted){fb::FileT file;file.id=it->second;file.path=it->first;record(std::move(file));}
 return it->second;
}
void FlatStream::flushBlock() {
 if(pending.empty())return;
 auto root=fb::CreateBlock(builder,builder.CreateVector(pending));
 fb::FinishSizePrefixedBlockBuffer(builder,root);
 output.write(reinterpret_cast<const char*>(builder.GetBufferPointer()),builder.GetSize());
 pending.clear();builder.Clear();
}
void FlatStream::finish() {
 if(finished)return;
 record(fb::EndT{});
 flushBlock();output.flush();finished=true;
}
}
