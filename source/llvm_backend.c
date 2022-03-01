#include "llvm_backend.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

b8 llvmvar_key_is_null(llvmvar_hash_table_key k) { return k.name.size == 0 && k.depth == 0; }
b8 llvmvar_key_is_eq(llvmvar_hash_table_key a, llvmvar_hash_table_key b) { return str_eq(a.name, b.name) && a.depth == b.depth; }
u32 hash_llvmvar_key(llvmvar_hash_table_key k) { return str_hash(k.name) + k.depth; }
b8 llvmvar_val_is_null(llvmvar_hash_table_value v) { return v.not_null == false; }
b8 llvmvar_val_is_tombstone(llvmvar_hash_table_value v) { return v.tombstone == true; }
HashTable_Impl(llvmvar, llvmvar_key_is_null, llvmvar_key_is_eq, hash_llvmvar_key, ((llvmvar_hash_table_value) { .type = (LLVMTypeRef) {0}, .alloca = (LLVMValueRef) {0}, .loaded = (LLVMValueRef) {0}, .not_null = false, .tombstone = true }), llvmvar_val_is_null, llvmvar_val_is_tombstone);

static void BL_Report(BL_Emitter* emitter, const char* stage, const char* err, ...) {
    fprintf(stderr, "%s Error: ", stage);
    va_list va;
    va_start(va, err);
    vfprintf(stderr, err, va);
    va_end(va);
}

#define BL_ReportCodegenError(emitter, error, ...) BL_Report(emitter, "Codegen", error, __VA_ARGS__)

static LLVMTypeRef printffunctype;
static LLVMValueRef printffunc;
static LLVMValueRef mainfunc;

static LLVMTypeRef BL_PTypeToLLVMType(P_Type* type) {
    switch (type->type) {
        case BasicType_Void: return LLVMVoidType();
        case BasicType_Integer: return LLVMInt64Type();
        case BasicType_Function: {
            LLVMTypeRef return_type = BL_PTypeToLLVMType(type->function.return_type);
            LLVMTypeRef* param_types = calloc(type->function.arity, sizeof(LLVMTypeRef));
            for (u32 i = 0; i < type->function.arity; i++)
                param_types[i] = BL_PTypeToLLVMType(type->function.param_types[i]);
            LLVMTypeRef ret = LLVMFunctionType(return_type, param_types, type->function.arity, false);
            free(param_types);
            return ret;
        } break;
    }
    return (LLVMTypeRef) {0};
}

void BL_Init(BL_Emitter* emitter, string filename) {
    emitter->filename = filename;
    
    emitter->module = LLVMModuleCreateWithName("Testing");
    
    LLVMTypeRef param_types[] = {};
    LLVMTypeRef function_type = LLVMFunctionType(LLVMInt32Type(), param_types, 0, 0);
    mainfunc = LLVMAddFunction(emitter->module, "main", function_type);
    emitter->entry = LLVMAppendBasicBlock(mainfunc, "entry");
    emitter->builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(emitter->builder, emitter->entry);
    
    LLVMTypeRef int_8_type_ptr = LLVMPointerType(LLVMInt8Type(), 0);
    LLVMTypeRef printf_function_args_type[] = { int_8_type_ptr };
    printffunctype = LLVMFunctionType(LLVMInt64Type(), printf_function_args_type, 1, true);
    printffunc = LLVMAddFunction(emitter->module, "printf", printffunctype);
    
    llvmvar_hash_table_init(&emitter->variables);
}

static LLVMValueRef BL_BuildBinary(BL_Emitter* emitter, L_Token token, LLVMValueRef left, LLVMValueRef right) {
    switch (token.type) {
        case TokenType_Plus:  return LLVMBuildAdd(emitter->builder, left, right, "");
        case TokenType_Minus: return LLVMBuildSub(emitter->builder, left, right, "");
        case TokenType_Star:  return LLVMBuildMul(emitter->builder, left, right, "");
        case TokenType_Slash: return LLVMBuildUDiv(emitter->builder, left, right, "");
        default: unreachable;
    }
    return (LLVMValueRef) {0};
}

static LLVMValueRef BL_BuildUnary(BL_Emitter* emitter, L_Token op, LLVMValueRef operand) {
    switch (op.type) {
        case TokenType_Plus:  return operand;
        case TokenType_Minus: return LLVMBuildNeg(emitter->builder, operand, "");
        case TokenType_Tilde: return LLVMBuildNot(emitter->builder, operand, "");
        default: unreachable;
    }
    return (LLVMValueRef) {0};
}

LLVMValueRef BL_Emit(BL_Emitter* emitter, AstNode* node) {
    switch (node->type) {
        case NodeType_Ident: {
            llvmvar_hash_table_key key = (llvmvar_hash_table_key) { .name = node->Ident.lexeme, .depth = 0 };
            llvmvar_hash_table_value val;
            if (llvmvar_hash_table_get(&emitter->variables, key, &val)) {
                return val.loaded;
            }
            return (LLVMValueRef) {0};
        }
        
        case NodeType_IntLit: return LLVMConstInt(LLVMInt64Type(), node->IntLit, 0);
        
        case NodeType_GlobalString: {
            static char global_string_name = 48;
            
            char* hoist = malloc(node->GlobalString.value.size + 1);
            memcpy(hoist, node->GlobalString.value.str, node->GlobalString.value.size);
            hoist[node->GlobalString.value.size] = '\0';
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
        
        case NodeType_Block: {
            for (u32 i = 0; i < node->Block.count; i++) {
                BL_Emit(emitter, node->Block.statements[i]);
            }
        }
        
        case NodeType_Assign: {
            char* hoist = malloc(node->Assign.name.lexeme.size + 1);
            memcpy(hoist, node->Assign.name.lexeme.str, node->Assign.name.lexeme.size);
            hoist[node->Assign.name.lexeme.size] = '\0';
            llvmvar_hash_table_key key = (llvmvar_hash_table_key) { .name = node->Assign.name.lexeme, .depth = 0 };
            llvmvar_hash_table_value val;
            if (llvmvar_hash_table_get(&emitter->variables, key, &val)) {
                LLVMValueRef value = BL_Emit(emitter, node->Assign.value);
                LLVMBuildStore(emitter->builder, value, val.alloca);
                val.loaded = LLVMBuildLoad2(emitter->builder, val.type, val.alloca, "");
                llvmvar_hash_table_set(&emitter->variables, key, val);
                
                free(hoist);
                return val.loaded;
            } else {
                free(hoist);
                return (LLVMValueRef) {0};
            }
        }
        
        case NodeType_VarDecl: {
            char* hoist = malloc(node->VarDecl.name.lexeme.size + 1);
            memcpy(hoist, node->VarDecl.name.lexeme.str, node->VarDecl.name.lexeme.size);
            hoist[node->VarDecl.name.lexeme.size] = '\0';
            LLVMTypeRef var_type = BL_PTypeToLLVMType(node->VarDecl.type);
            
            LLVMValueRef alloca = LLVMBuildAlloca(emitter->builder, var_type, hoist);
            if (node->VarDecl.value) {
                LLVMValueRef value = BL_Emit(emitter, node->VarDecl.value);
                LLVMBuildStore(emitter->builder, value, alloca);
            }
            LLVMValueRef loaded = LLVMBuildLoad2(emitter->builder, var_type, alloca, "");
            
            llvmvar_hash_table_key key = (llvmvar_hash_table_key) { .name = node->VarDecl.name.lexeme, .depth = 0 };
            llvmvar_hash_table_value val = (llvmvar_hash_table_value) { .alloca = alloca, .loaded = loaded, .type = var_type, .not_null = true, .tombstone = false };
            llvmvar_hash_table_set(&emitter->variables, key, val);
            
            free(hoist);
            return alloca;
        }
    }
    return (LLVMValueRef) {0};
}

void BL_Free(BL_Emitter* emitter) {
    LLVMTypeRef int_8_type_ptr = LLVMPointerType(LLVMInt8Type(), 0);
    
    llvmvar_hash_table_key key = (llvmvar_hash_table_key) { .name = str_lit("m"), .depth = 0 };
    llvmvar_hash_table_value val;
    llvmvar_hash_table_get(&emitter->variables, key, &val);
    LLVMValueRef printfargs[] = {
        LLVMBuildPointerCast(emitter->builder,
                             LLVMBuildGlobalString(emitter->builder, "%lld\n", ""),
                             int_8_type_ptr, "0"),
        val.loaded,
    };
    
    LLVMBuildCall2(emitter->builder, printffunctype, printffunc, printfargs, 2, "");
    LLVMBuildRet(emitter->builder, LLVMConstInt(LLVMInt32Type(), 0, false));
    
    char* error = nullptr;
    LLVMVerifyModule(emitter->module, LLVMPrintMessageAction, &error);
    if (error) {
        printf("%s", error);
        LLVMDisposeMessage(error);
        error = nullptr;
    }
    
    llvmvar_hash_table_free(&emitter->variables);
    
    LLVMPrintModuleToFile(emitter->module, (char*)emitter->filename.str, &error);
    
    LLVMDisposeBuilder(emitter->builder);
    LLVMDisposeModule(emitter->module);
}
