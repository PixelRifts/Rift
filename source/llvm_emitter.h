/* date = October 10th 2022 9:04 pm */

#ifndef LLVM_EMITTER_H
#define LLVM_EMITTER_H

#include "ast_nodes.h"
#include "llvm-c/Core.h"

#if 0
typedef struct LLVMModuleRef LLVMModuleRef;
typedef struct LLVMTypeRef LLVMTypeRef;
typedef struct LLVMValueRef LLVMValueRef;
typedef struct LLVMBuilderRef LLVMBuilderRef;
typedef struct LLVMTargetRef LLVMTargetRef;
typedef struct LLVMTargetMachineRef LLVMTargetMachineRef;
typedef struct LLVMTargetDataRef LLVMTargetDataRef;
typedef struct LLVMContextRef LLVMContextRef;
typedef struct LLVMBasicBlockRef LLVMBasicBlockRef;
#endif

typedef struct LLVM_Emitter {
	LLVMContextRef context;
	LLVMModuleRef module;
	LLVMBuilderRef builder;
	
	LLVMTypeRef int_8_type;
	LLVMTypeRef int_8_type_ptr;
	LLVMTypeRef int_32_type;
	
	LLVMTypeRef printf_type;
	LLVMValueRef printf_object;
} LLVM_Emitter;

LLVMValueRef LLVM_Emit(LLVM_Emitter* emitter, IR_Ast* ast);

void LLVM_Init(LLVM_Emitter* emitter);
void LLVM_Free(LLVM_Emitter* emitter);

#endif //LLVM_EMITTER_H
