/* date = February 19th 2022 7:25 am */

#ifndef LLVM_BACKEND_H
#define LLVM_BACKEND_H

#include "checker.h"
#include "ds.h"
#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"

typedef struct BL_StackValue {
    LLVMTypeRef type;
    LLVMValueRef alloca;
    LLVMValueRef loaded;
    b8 not_null;
    b8 tombstone;
} BL_StackValue;

HashTable_Prototype(llvmsymbol, struct { string name; u32 depth; }, BL_StackValue);

typedef struct BL_Emitter {
    string filename;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMBasicBlockRef current_block;
    b8 is_in_function;
    
    llvmsymbol_hash_table variables;
} BL_Emitter;

void BL_Init(BL_Emitter* emitter, string filename);
LLVMValueRef BL_Emit(BL_Emitter* emitter, AstNode* node);
void BL_Free(BL_Emitter* emitter);

#endif //LLVM_BACKEND_H
