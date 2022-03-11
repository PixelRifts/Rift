/* date = February 19th 2022 7:25 am */

#ifndef LLVM_BACKEND_H
#define LLVM_BACKEND_H

#include "checker.h"
#include "ds.h"
#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Error.h"

typedef u32 BL_ValueFlag;
enum {
    ValueFlag_Global = 0x1,
};

typedef struct BL_Value {
    C_SymbolType symtype;
    BL_ValueFlag symflags;
    
    LLVMTypeRef type;
    LLVMValueRef alloca;
    LLVMValueRef loaded;
    b8 changed;
    b8 not_null;
    b8 tombstone;
} BL_Value;

typedef struct BL_Metadata {
    LLVMMetadataRef metadata;
    
    b8 not_null;
    b8 tombstone;
} BL_Metadata;

HashTable_Prototype(llvmsymbol, struct { string name; u32 depth; }, BL_Value);
HashTable_Prototype(llvmmeta, void*, BL_Metadata);

typedef struct BL_BlockContext BL_BlockContext;
struct BL_BlockContext {
    BL_BlockContext* prev;
    LLVMBasicBlockRef basic_block;
};

typedef struct BL_Emitter {
    string filename;
    string source_filename;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMDIBuilderRef debug_builder;
    LLVMBasicBlockRef current_block;
    b8 is_in_function;
    
    LLVMValueRef func_return;
    LLVMBasicBlockRef return_block;
    b8 emitted_end_in_this_block;
    
    BL_BlockContext* current_block_context;
    llvmsymbol_hash_table variables;
} BL_Emitter;

void BL_Init(BL_Emitter* emitter, string source_filename, string filename);
LLVMValueRef BL_Emit(BL_Emitter* emitter, AstNode* node);
void BL_Free(BL_Emitter* emitter);

#endif //LLVM_BACKEND_H
