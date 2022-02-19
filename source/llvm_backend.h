/* date = February 19th 2022 7:25 am */

#ifndef LLVM_BACKEND_H
#define LLVM_BACKEND_H

#include "parser.h"
#include "llvm-c/Core.h"
typedef struct BL_Emitter {
    string filename;
    LLVMModuleRef module;
    LLVMBasicBlockRef entry;
    LLVMBuilderRef builder;
} BL_Emitter;

void BL_Init(BL_Emitter* emitter, string filename);
LLVMValueRef BL_Emit(BL_Emitter* emitter, AstNode* node);
void BL_Free(BL_Emitter* emitter);

#endif //LLVM_BACKEND_H
