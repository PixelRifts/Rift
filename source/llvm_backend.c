#include "llvm_backend.h"
#include <stdio.h>
#include <stdarg.h>

static void BL_Report(BL_Emitter* emitter, const char* stage, const char* err, ...) {
    fprintf(stderr, "%s Error: ", stage);
    va_list va;
    va_start(va, err);
    vfprintf(stderr, err, va);
    va_end(va);
}

#define BL_ReportLexError(emitter, error, ...) BL_Report(emitter, "Codegen", error, __VA_ARGS__)

void BL_Init(BL_Emitter* emitter, string filename) {
    emitter->filename = filename;
    
    emitter->module = LLVMModuleCreateWithName("Testing");
    
    LLVMTypeRef param_types[] = {};
    LLVMTypeRef function_type = LLVMFunctionType(LLVMInt64Type(), param_types, 0, 0);
    LLVMValueRef fn = LLVMAddFunction(emitter->module, "main", function_type);
    emitter->entry = LLVMAppendBasicBlock(fn, "entry");
    emitter->builder  = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(emitter->builder, emitter->entry);
}

LLVMValueRef BL_BuildBinary(BL_Emitter* emitter, L_Token token, LLVMValueRef left, LLVMValueRef right) {
    static char ctr = 'a' - 1;
    switch (token.type) {
        case TokenType_Plus:  ctr++; return LLVMBuildAdd(emitter->builder, left, right, &ctr);
        case TokenType_Minus: ctr++; return LLVMBuildSub(emitter->builder, left, right, &ctr);
        case TokenType_Star:  ctr++; return LLVMBuildMul(emitter->builder, left, right, &ctr);
        case TokenType_Slash: ctr++; return LLVMBuildUDiv(emitter->builder, left, right, &ctr);
    }
    return (LLVMValueRef) {0};
}

LLVMValueRef BL_Emit(BL_Emitter* emitter, AstNode* node) {
    switch (node->type) {
        case NodeType_Ident: return (LLVMValueRef) {0};
        case NodeType_IntLit: return LLVMConstInt(LLVMInt64Type(), node->IntLit, 0);
        case NodeType_Binary: {
            LLVMValueRef left = BL_Emit(emitter, node->Binary.left);
            LLVMValueRef right = BL_Emit(emitter, node->Binary.right);
            return BL_BuildBinary(emitter, node->Binary.op, left, right);
        }
        case NodeType_Return: return LLVMBuildRet(emitter->builder, BL_Emit(emitter, node->Return));
    }
    return (LLVMValueRef) {0};
}

void BL_Free(BL_Emitter* emitter) {
    //LLVMBuildRet(emitter->builder, LLVMConstInt(LLVMInt64Type(), 0, 0));
    char* error = nullptr;
    LLVMPrintModuleToFile(emitter->module, (char*)emitter->filename.str, &error);
}
