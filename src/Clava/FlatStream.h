#pragma once
#include "FlatSupport.h"
#include "llvm/Support/raw_ostream.h"
#include <unordered_map>
namespace clava::flat {
fb::NodeT makeNode(const clang::Decl *,Context &);
fb::NodeT makeNode(const clang::Stmt *,Context &);
fb::NodeT makeNode(const clang::Expr *,Context &);
fb::NodeT makeNode(const clang::Type *,Context &);
fb::NodeT makeNode(const clang::Attr *,Context &);
fb::NodeT makeNode(const clang::QualType &,Context &);

/** Incremental size-prefixed blocks sharing vtables across records. */
class FlatStream final : public llvm::raw_ostream {
 llvm::raw_ostream &output;
 flatbuffers::FlatBufferBuilder builder;
 std::unordered_map<std::string,uint32_t> files;
 std::vector<flatbuffers::Offset<fb::Record>> pending;
 uint64_t records=0,nodes=0,bytes=0;
 void flushBlock();
 bool finished=false;
 void write_impl(const char *,size_t) override;
 uint64_t current_pos() const override {return bytes;}
 uint32_t fileId(llvm::StringRef path);
public:
 explicit FlatStream(llvm::raw_ostream &output);
 ~FlatStream() override;
 static FlatStream *active();
 template<class T> void record(T value) {
   fb::RecordT record;record.payload.Set(std::move(value));
   pending.push_back(fb::Record::Pack(builder,&record));
   records++;
   if(builder.GetSize()>=64*1024)flushBlock();
 }
 template<class T> void node(T node,clang::ASTContext *ast,int id) {
   Context c{ast,id,[this](llvm::StringRef p){return fileId(p);}};
   record(makeNode(node,c));nodes++;
 }
 void finish();
};
}
