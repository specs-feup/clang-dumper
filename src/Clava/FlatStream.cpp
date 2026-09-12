#include "FlatStream.h"
#include "FlatSchemaHash.h"
namespace clava::flat {
namespace { FlatStream *activeStream=nullptr; }
#include "FlatDispatch.inc"
FlatStream::FlatStream(llvm::raw_ostream &output):output(output) {
 if(activeStream)throw std::logic_error("Nested FlatBuffers dump streams");
 SetUnbuffered();activeStream=this;fb::HeaderT header;header.schema_hash=SchemaHash;record(std::move(header));
}
FlatStream::~FlatStream(){if(activeStream==this)activeStream=nullptr;}
FlatStream *FlatStream::active(){return activeStream;}
void FlatStream::write_impl(const char *,size_t size) {
 if(size)throw std::logic_error("Legacy text reached the complete FlatBuffers writer");
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
 bytes+=builder.GetSize();pending.clear();builder.Clear();
}
void FlatStream::finish() {
 if(finished)return;
 fb::EndT end;end.records=records;end.nodes=nodes;end.files=files.size();end.ids=clava::denseIdCount();record(std::move(end));
 flushBlock();output.flush();finished=true;
}
bool emit(const clang::Decl * node,clang::ASTContext *ast,int id) { if(auto *s=FlatStream::active()){s->node(node,ast,id);return true;}return false;}
bool emit(const clang::Stmt * node,clang::ASTContext *ast,int id) { if(auto *s=FlatStream::active()){s->node(node,ast,id);return true;}return false;}
bool emit(const clang::Expr * node,clang::ASTContext *ast,int id) { if(auto *s=FlatStream::active()){s->node(node,ast,id);return true;}return false;}
bool emit(const clang::Type * node,clang::ASTContext *ast,int id) { if(auto *s=FlatStream::active()){s->node(node,ast,id);return true;}return false;}
bool emit(const clang::Attr * node,clang::ASTContext *ast,int id) { if(auto *s=FlatStream::active()){s->node(node,ast,id);return true;}return false;}
bool emit(const clang::QualType & node,clang::ASTContext *ast,int id) { if(auto *s=FlatStream::active()){s->node(node,ast,id);return true;}return false;}
}
