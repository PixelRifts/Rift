#include "llvm_backend.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

b8 llvmsymbol_key_is_null(llvmsymbol_hash_table_key k) { return k.name.size == 0 && k.depth == 0; }
b8 llvmsymbol_key_is_eq(llvmsymbol_hash_table_key a, llvmsymbol_hash_table_key b) { return str_eq(a.name, b.name) && a.depth == b.depth; }
u32 hash_llvmsymbol_key(llvmsymbol_hash_table_key k) { return str_hash(k.name) + k.depth; }
b8 llvmsymbol_val_is_null(llvmsymbol_hash_table_value v) { return v.not_null == false; }
b8 llvmsymbol_val_is_tombstone(llvmsymbol_hash_table_value v) { return v.tombstone == true; }
HashTable_Impl(llvmsymbol, llvmsymbol_key_is_null, llvmsymbol_key_is_eq, hash_llvmsymbol_key, ((llvmsymbol_hash_table_value) { .type = (LLVMTypeRef) {0}, .alloca = (LLVMValueRef) {0}, .loaded = (LLVMValueRef) {0}, .not_null = false, .tombstone = true }), llvmsymbol_val_is_null, llvmsymbol_val_is_tombstone);

static void BL_Report(BL_Emitter* emitter, const char* stage, const char* err, ...) {
    fprintf(stderr, "%.*s:%s ERROR: ", str_expand(emitter->source_filename), stage);
    va_list va;
    va_start(va, err);
    vfprintf(stderr, err, va);
    va_end(va);
}

static void BL_Report_LLVMHandler(const char* reason) { fprintf(stderr, "LLVM Error %s", reason); }

#define BL_ReportCodegenError(emitter, error, ...) BL_Report(emitter, "Codegen", error, __VA_ARGS__)

LLVMTypeRef printffunctype;
LLVMValueRef printffunc;

static LLVMTypeRef BL_PTypeToLLVMType(P_Type* type) {
    switch (type->type) {
        case BasicType_Void: return LLVMVoidType();
        case BasicType_Integer: return LLVMInt64Type();
        case BasicType_Boolean: return LLVMInt1Type();
        case BasicType_Cstring: return LLVMPointerType(LLVMInt8Type(), 0);
        case BasicType_Function: {
            LLVMTypeRef return_type = BL_PTypeToLLVMType(type->function.return_type);
            LLVMTypeRef* param_types = calloc(type->function.arity, sizeof(LLVMTypeRef));
            for (u32 i = 0; i < type->function.arity; i++)
                param_types[i] = BL_PTypeToLLVMType(type->function.param_types[i]);
            LLVMTypeRef ret = LLVMFunctionType(return_type, param_types, type->function.arity, type->function.varargs);
            free(param_types);
            return ret;
        } break;
    }
    return (LLVMTypeRef) {0};
}

#define INITIALIZE_TARGET(X) do { \
LLVMInitialize ## X ## AsmParser(); \
LLVMInitialize ## X ## AsmPrinter(); \
LLVMInitialize ## X ## TargetInfo(); \
LLVMInitialize ## X ## Target(); \
LLVMInitialize ## X ## Disassembler(); \
LLVMInitialize ## X ## TargetMC(); \
} while(0)

void BL_Init(BL_Emitter* emitter, string source_filename, string filename) {
    memset(emitter, 0, sizeof(BL_Emitter));
    emitter->filename = filename;
    emitter->source_filename = source_filename;
    emitter->module = LLVMModuleCreateWithName("Testing");
    emitter->builder = LLVMCreateBuilder();
    
    INITIALIZE_TARGET(X86);
    
    LLVMTargetRef target;
    const char* error = nullptr;
    if (LLVMGetTargetFromTriple(LLVMGetDefaultTargetTriple(), &target, &error) != 0) {
        printf("Target From Triple: %s\n", error);
        fflush(stdout);
    }
    LLVMSetTarget(emitter->module, LLVMGetDefaultTargetTriple());
    LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(target, LLVMGetDefaultTargetTriple(), "", "", LLVMCodeGenLevelNone, LLVMRelocDefault, LLVMCodeModelDefault);
    LLVMTargetDataRef target_data = LLVMCreateTargetDataLayout(target_machine);
    LLVMSetModuleDataLayout(emitter->module, target_data);
    
    llvmsymbol_hash_table_init(&emitter->variables);
    
    LLVMTypeRef printf_function_args_type[] = { LLVMPointerType(LLVMInt8Type(), 0) };
    LLVMTypeRef rintffunctype = LLVMFunctionType(LLVMInt64Type(), printf_function_args_type, 1, true);
    LLVMValueRef rintffunc = LLVMAddFunction(emitter->module, "printf", rintffunctype);
    
    llvmsymbol_hash_table_key key = (llvmsymbol_hash_table_key) { .name = str_lit("printf"), .depth = 0 };
    llvmsymbol_hash_table_value val = { .type = rintffunctype, .alloca = rintffunc, .loaded = nullptr, .not_null = true };
    llvmsymbol_hash_table_set(&emitter->variables, key, val);
    
    LLVMInstallFatalErrorHandler(BL_Report_LLVMHandler);
    LLVMEnablePrettyStackTrace();
}

static LLVMValueRef BL_BuildBinary(BL_Emitter* emitter, L_Token token, LLVMValueRef left, LLVMValueRef right) {
    switch (token.type) {
        case TokenType_Plus:  return LLVMBuildAdd(emitter->builder, left, right, "");
        case TokenType_Minus: return LLVMBuildSub(emitter->builder, left, right, "");
        case TokenType_Star:  return LLVMBuildMul(emitter->builder, left, right, "");
        case TokenType_Slash: return LLVMBuildUDiv(emitter->builder, left, right, "");
        
        case TokenType_AmpersandAmpersand: return LLVMBuildBitCast(emitter->builder, LLVMBuildAnd(emitter->builder, left, right, ""), LLVMInt1Type(), "");
        case TokenType_PipePipe: return LLVMBuildBitCast(emitter->builder, LLVMBuildOr(emitter->builder, left, right, ""), LLVMInt1Type(), "");
        
        case TokenType_EqualEqual: {
            if (LLVMGetTypeKind(LLVMTypeOf(left)) == LLVMIntegerTypeKind)
                return LLVMBuildICmp(emitter->builder, LLVMIntEQ, left, right, "");
        }
        
        case TokenType_BangEqual: {
            if (LLVMGetTypeKind(LLVMTypeOf(left)) == LLVMIntegerTypeKind)
                return LLVMBuildICmp(emitter->builder, LLVMIntNE, left, right, "");
        }
        
        case TokenType_Less: {
            if (LLVMGetTypeKind(LLVMTypeOf(left)) == LLVMIntegerTypeKind)
                return LLVMBuildICmp(emitter->builder, LLVMIntSLT, left, right, "");
        }
        
        case TokenType_LessEqual: {
            if (LLVMGetTypeKind(LLVMTypeOf(left)) == LLVMIntegerTypeKind)
                return LLVMBuildICmp(emitter->builder, LLVMIntSLE, left, right, "");
        }
        
        case TokenType_Greater: {
            if (LLVMGetTypeKind(LLVMTypeOf(left)) == LLVMIntegerTypeKind)
                return LLVMBuildICmp(emitter->builder, LLVMIntSGT, left, right, "");
        }
        
        case TokenType_GreaterEqual: {
            if (LLVMGetTypeKind(LLVMTypeOf(left)) == LLVMIntegerTypeKind)
                return LLVMBuildICmp(emitter->builder, LLVMIntSGE, left, right, "");
        }
        
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
            llvmsymbol_hash_table_key key = (llvmsymbol_hash_table_key) { .name = node->Ident, .depth = 0 };
            llvmsymbol_hash_table_value val;
            if (llvmsymbol_hash_table_get(&emitter->variables, key, &val)) {
                return val.loaded;
            }
            return (LLVMValueRef) {0};
        }
        
        case NodeType_IntLit: return LLVMConstInt(LLVMInt64Type(), node->IntLit, 0);
        case NodeType_BoolLit: return LLVMConstInt(LLVMInt1Type(), node->BoolLit, 0);
        
        case NodeType_GlobalString: {
            char* hoist = malloc(node->GlobalString.value.size + 1);
            memcpy(hoist, node->GlobalString.value.str, node->GlobalString.value.size);
            hoist[node->GlobalString.value.size] = '\0';
            LLVMValueRef ret = LLVMBuildPointerCast(emitter->builder, LLVMBuildGlobalString(emitter->builder, hoist, ""), LLVMPointerType(LLVMInt8Type(), 0), "");
            free(hoist);
            
            return ret;
        } break;
        
        case NodeType_Unary: {
            LLVMValueRef operand = BL_Emit(emitter, node->Unary.expr);
            return BL_BuildUnary(emitter, node->id, operand);
        }
        
        case NodeType_Binary: {
            LLVMValueRef left = BL_Emit(emitter, node->Binary.left);
            LLVMValueRef right = BL_Emit(emitter, node->Binary.right);
            return BL_BuildBinary(emitter, node->id, left, right);
        }
        
        case NodeType_Group: {
            return BL_Emit(emitter, node->Group);
        }
        
        case NodeType_Lambda: {
            LLVMBasicBlockRef prev = emitter->current_block;
            b8 prev_is_in_function = emitter->is_in_function;
            
            LLVMTypeRef function_type = BL_PTypeToLLVMType(node->Lambda.function_type);
            LLVMValueRef func = LLVMAddFunction(emitter->module, "", function_type);
            
            emitter->current_block = LLVMAppendBasicBlock(func, "entry");
            LLVMPositionBuilderAtEnd(emitter->builder, emitter->current_block);
            emitter->is_in_function = true;
            
            for (u32 i = 0; i < node->Lambda.function_type->function.arity; i++) {
                string var_name = node->Lambda.param_names[i];
                LLVMTypeRef type = BL_PTypeToLLVMType(node->Lambda.function_type->function.param_types[i]);
                
                LLVMValueRef alloca = LLVMBuildAlloca(emitter->builder, type, "");
                LLVMBuildStore(emitter->builder, LLVMGetParam(func, i), alloca);
                LLVMValueRef loaded = LLVMBuildLoad(emitter->builder, alloca, "");
                
                llvmsymbol_hash_table_key key = (llvmsymbol_hash_table_key) { .name = var_name, .depth = 0 };
                llvmsymbol_hash_table_value val = { .type = type, .alloca = alloca, .loaded = loaded, .not_null = true };
                llvmsymbol_hash_table_set(&emitter->variables, key, val);
            }
            
            BL_Emit(emitter, node->Lambda.body);
            
            if (node->Lambda.function_type->function.return_type->type == BasicType_Void)
                LLVMBuildRet(emitter->builder, (LLVMValueRef) {0});
            
            emitter->is_in_function = prev_is_in_function;
            emitter->current_block = prev;
            LLVMPositionBuilderAtEnd(emitter->builder, emitter->current_block);
            return func;
        }
        
        case NodeType_Call: {
            // Change key.name when implementing function pointers
            
            llvmsymbol_hash_table_key key = (llvmsymbol_hash_table_key) { .name = node->Call.callee->id.lexeme, .depth = 0 };
            llvmsymbol_hash_table_value val;
            llvmsymbol_hash_table_get(&emitter->variables, key, &val);
            
            LLVMValueRef* params = malloc(node->Call.arity * sizeof(LLVMValueRef));
            for (u32 i = 0; i < node->Call.arity; i++) {
                params[i] = BL_Emit(emitter, node->Call.params[i]);
            }
            return LLVMBuildCall2(emitter->builder, val.type, val.alloca, params, node->Call.arity, "");
        }
        
        case NodeType_Return: {
            if (!node->Return) {
                return LLVMBuildRet(emitter->builder, (LLVMValueRef) {0});
            }
            return LLVMBuildRet(emitter->builder, BL_Emit(emitter, node->Return));
        }
        
        case NodeType_ExprStatement: {
            return BL_Emit(emitter, node->ExprStatement);
        }
        
        case NodeType_Block: {
            for (u32 i = 0; i < node->Block.count; i++) {
                BL_Emit(emitter, node->Block.statements[i]);
            }
            return (LLVMValueRef) {0};
        }
        
        case NodeType_If: {
            LLVMValueRef condition = BL_Emit(emitter, node->If.condition);
            
            //LLVMBasicBlockRef prev = emitter->current_block;
            
            LLVMValueRef current_function = LLVMGetBasicBlockParent(emitter->current_block);
            LLVMBasicBlockRef ifb = LLVMAppendBasicBlock(current_function, "th");
            LLVMBasicBlockRef elseb = nullptr;
            if (node->If.elsee) elseb = LLVMAppendBasicBlock(current_function, "el");
            LLVMBasicBlockRef mergeb = LLVMAppendBasicBlock(current_function, "me");
            if (!elseb) elseb = mergeb;
            
            LLVMBuildCondBr(emitter->builder, condition, ifb, elseb);
            
            LLVMPositionBuilderAtEnd(emitter->builder, ifb);
            emitter->current_block = ifb;
            BL_Emit(emitter, node->If.then);
            LLVMBuildBr(emitter->builder, mergeb);
            
            if (node->If.elsee) {
                LLVMPositionBuilderAtEnd(emitter->builder, elseb);
                emitter->current_block = elseb;
                BL_Emit(emitter, node->If.elsee);
                LLVMBuildBr(emitter->builder, mergeb);
            }
            
            LLVMPositionBuilderAtEnd(emitter->builder, mergeb);
            emitter->current_block = mergeb;
            
            return (LLVMValueRef) {0};
        }
        
        case NodeType_Assign: {
            char* hoist = malloc(node->id.lexeme.size + 1);
            memcpy(hoist, node->id.lexeme.str, node->id.lexeme.size);
            hoist[node->id.lexeme.size] = '\0';
            llvmsymbol_hash_table_key key = (llvmsymbol_hash_table_key) { .name = node->id.lexeme, .depth = 0 };
            llvmsymbol_hash_table_value val;
            if (llvmsymbol_hash_table_get(&emitter->variables, key, &val)) {
                LLVMValueRef value = BL_Emit(emitter, node->Assign.value);
                LLVMBuildStore(emitter->builder, value, val.alloca);
                val.loaded = LLVMBuildLoad2(emitter->builder, val.type, val.alloca, "");
                llvmsymbol_hash_table_set(&emitter->variables, key, val);
                
                free(hoist);
                return val.loaded;
            } else {
                free(hoist);
                return (LLVMValueRef) {0};
            }
        }
        
        case NodeType_VarDecl: {
            char* hoist = malloc(node->id.lexeme.size + 1);
            memcpy(hoist, node->id.lexeme.str, node->id.lexeme.size);
            hoist[node->id.lexeme.size] = '\0';
            
            LLVMValueRef addr = {0};
            LLVMValueRef ref = {0};
            
            LLVMTypeRef var_type = BL_PTypeToLLVMType(node->VarDecl.type);
            if (node->VarDecl.type->type == BasicType_Function && node->VarDecl.value->type != NodeType_Lambda)
                var_type = LLVMPointerType(var_type, 0);
            
            if (emitter->is_in_function) {
                
                addr = LLVMBuildAlloca(emitter->builder, var_type, hoist);
                if (node->VarDecl.value) {
                    if (node->VarDecl.value->type == NodeType_Lambda) {
                        addr = BL_Emit(emitter, node->VarDecl.value);
                        LLVMSetValueName2(addr, (const char*)node->id.lexeme.str, node->id.lexeme.size);
                    } else {
                        BL_Emit(emitter, node->VarDecl.value);
                    }
                } else {
                    // TODO(voxel): @default_init
                }
                ref = LLVMBuildLoad2(emitter->builder, var_type, addr, "");
                
            } else {
                if (node->VarDecl.value) {
                    if (node->VarDecl.value->type == NodeType_Lambda) {
                        addr = BL_Emit(emitter, node->VarDecl.value);
                        LLVMSetValueName2(addr, (const char*)node->id.lexeme.str, node->id.lexeme.size);
                    } else {
                        addr = LLVMAddGlobal(emitter->module, var_type, hoist);
                        LLVMSetInitializer(addr, BL_Emit(emitter, node->VarDecl.value));
                    }
                } else {
                    addr = LLVMAddGlobal(emitter->module, var_type, hoist);
                    // TODO(voxel): @default_init
                }
            }
            
            llvmsymbol_hash_table_key key = (llvmsymbol_hash_table_key) { .name = node->id.lexeme, .depth = 0 };
            llvmsymbol_hash_table_value val = (llvmsymbol_hash_table_value) { .alloca = addr, .loaded = ref, .type = var_type, .not_null = true, .tombstone = false };
            llvmsymbol_hash_table_set(&emitter->variables, key, val);
            
            free(hoist);
            return addr;
        }
    }
    return (LLVMValueRef) {0};
}

void BL_Free(BL_Emitter* emitter) {
    char* error = nullptr;
    LLVMVerifyModule(emitter->module, LLVMPrintMessageAction, nullptr);
    
#if 1
    LLVMPrintModuleToFile(emitter->module, (char*)emitter->filename.str, &error);
#else
    LLVMExecutionEngineRef engine;
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    if (LLVMCreateExecutionEngineForModule(&engine, emitter->module, &error) != 0) {
        fprintf(stderr, "Failed to create execution engine\n");
        fflush(stderr);
    }
    if (error) {
        fprintf(stderr, "Execution Engine Error: %s\n", error);
        fflush(stderr);
        LLVMDisposeErrorMessage(error);
    }
    
    llvmsymbol_hash_table_key key = (llvmsymbol_hash_table_key) { .name = str_lit("main"), .depth = 0 };
    llvmsymbol_hash_table_value val;
    llvmsymbol_hash_table_get(&emitter->variables, key, &val);
    
    LLVMRunFunction(engine, val.alloca, 0, nullptr);
    
#endif
    llvmsymbol_hash_table_free(&emitter->variables);
    
    LLVMDisposeModule(emitter->module);
    LLVMDisposeBuilder(emitter->builder);
}
