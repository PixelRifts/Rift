/* date = February 19th 2022 7:25 am */

#ifndef LLVM_BACKEND_H
#define LLVM_BACKEND_H

#include "parser.h"
#include "ds.h"
#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"

typedef struct BL_StackValue {
    LLVMValueRef alloca;
    LLVMValueRef loaded;
    b8 not_null;
    b8 tombstone;
} BL_StackValue;

HashTable_Prototype(llvmvar, struct { string name; u32 depth; }, BL_StackValue);

typedef struct BL_Emitter {
    string filename;
    LLVMModuleRef module;
    LLVMBasicBlockRef entry;
    LLVMBuilderRef builder;
    
    llvmvar_hash_table variables;
} BL_Emitter;

void BL_Init(BL_Emitter* emitter, string filename);
LLVMValueRef BL_Emit(BL_Emitter* emitter, AstNode* node);
void BL_Free(BL_Emitter* emitter);

#endif //LLVM_BACKEND_H
