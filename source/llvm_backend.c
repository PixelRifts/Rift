#include "llvm_backend.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

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
    
    LLVMTypeRef puts_function_args_type[] = { LLVMInt8Type() };
    LLVMTypeRef puts_function_type = LLVMFunctionType(LLVMInt64Type(), puts_function_args_type, 1, false);
    LLVMAddFunction(emitter->module, "puts", puts_function_type);
}

static char ctr = 'a';

static LLVMValueRef BL_BuildBinary(BL_Emitter* emitter, L_Token token, LLVMValueRef left, LLVMValueRef right) {
    ctr++;
    switch (token.type) {
        case TokenType_Plus:  return LLVMBuildAdd(emitter->builder, left, right, &ctr);
        case TokenType_Minus: return LLVMBuildSub(emitter->builder, left, right, &ctr);
        case TokenType_Star:  return LLVMBuildMul(emitter->builder, left, right, &ctr);
        case TokenType_Slash: return LLVMBuildUDiv(emitter->builder, left, right, &ctr);
        default: unreachable;
    }
    return (LLVMValueRef) {0};
}

static LLVMValueRef BL_BuildUnary(BL_Emitter* emitter, L_Token op, LLVMValueRef operand) {
    ctr++;
    switch (op.type) {
        case TokenType_Plus:  return operand;
        case TokenType_Minus: return LLVMBuildNeg(emitter->builder, operand, &ctr);
        case TokenType_Tilde: return LLVMBuildNot(emitter->builder, operand, &ctr);
        default: unreachable;
    }
    return (LLVMValueRef) {0};
}

LLVMValueRef BL_Emit(BL_Emitter* emitter, AstNode* node) {
    switch (node->type) {
        case NodeType_Ident: return (LLVMValueRef) {0};
        
        case NodeType_IntLit: return LLVMConstInt(LLVMInt64Type(), node->IntLit, 0);
        
        case NodeType_GlobalString: {
            static char global_string_name = 48;
            
            char* hoist = malloc(node->GlobalString.size + 1);
            memcpy(hoist, node->GlobalString.str, node->GlobalString.size);
            hoist[node->GlobalString.size] = '\0';
            LLVMValueRef ret = LLVMBuildGlobalString(emitter->builder, hoist, &global_string_name);
            global_string_name++;
            free(hoist);
            
            return ret;
        } break;
        
        case NodeType_Unary: {
            LLVMValueRef operand = BL_Emit(emitter, node->Unary.expr);
            return BL_BuildUnary(emitter, node->Unary.op, operand);
        }
        
        case NodeType_Binary: {
            LLVMValueRef left = BL_Emit(emitter, node->Binary.left);
            LLVMValueRef right = BL_Emit(emitter, node->Binary.right);
            return BL_BuildBinary(emitter, node->Binary.op, left, right);
        }
        
        case NodeType_Return: return LLVMBuildRet(emitter->builder, BL_Emit(emitter, node->Return));
        
        case NodeType_VarDecl: {
            char* hoist = malloc(node->VarDecl.name.length + 1);
            memcpy(hoist, node->VarDecl.name.start, node->VarDecl.name.length);
            hoist[node->VarDecl.name.length] = '\0';
            LLVMValueRef alloca = LLVMBuildAlloca(emitter->builder, LLVMInt64Type(), hoist);
            free(hoist);
            if (node->VarDecl.value) {
                LLVMValueRef value = BL_Emit(emitter, node->VarDecl.value);
                LLVMBuildStore(emitter->builder, value, alloca);
            }
            return alloca;
        }
    }
    return (LLVMValueRef) {0};
}

void BL_Free(BL_Emitter* emitter) {
    char* error = nullptr;
    LLVMPrintModuleToFile(emitter->module, (char*)emitter->filename.str, &error);
    LLVMDisposeBuilder(emitter->builder);
    LLVMDisposeModule(emitter->module);
}
