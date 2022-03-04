/*
 TODOS(voxel):
- Better Error Output. Should show line with ----^ underneath offending token
*/

#include "checker.h"
#include "checker_data.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

//~ Data structures

b8 symbol_key_is_null(symbol_hash_table_key k) { return k.name.size == 0 && k.depth == 0; }
b8 symbol_key_is_eq(symbol_hash_table_key a, symbol_hash_table_key b) { return str_eq(a.name, b.name) && a.depth == b.depth; }
u32 hash_symbol_key(symbol_hash_table_key k) { return str_hash(k.name) + k.depth; }
b8 symbol_val_is_null(symbol_hash_table_value v) { return v.type == SymbolType_Invalid; }
b8 symbol_val_is_tombstone(symbol_hash_table_value v) { return v.type == SymbolType_Count; }
HashTable_Impl(symbol, symbol_key_is_null, symbol_key_is_eq, hash_symbol_key, (symbol_hash_table_value) { .type = SymbolType_Count }, symbol_val_is_null, symbol_val_is_tombstone);

//~ Diagnostics

static void C_Report(C_Checker* checker, L_Token token, const char* stage, const char* err, ...) {
    if (checker->errored) return;
    if (checker->error_count > 20) exit(-1);
    fprintf(stderr, "%s Error:%d:%d: ", stage, token.line, token.column);
    va_list va;
    va_start(va, err);
    vfprintf(stderr, err, va);
    va_end(va);
    checker->errored = true;
}

#define C_ReportCheckError(checker, token, error, ...) C_Report(checker, token, "Checker", error, __VA_ARGS__)

//~ Type Stuff

#define TYPE(Id, Name) case BasicType_##Id: return Name;
static string C_GetBasicTypeName(P_Type* type) {
    if (!type) return str_lit("Invalid");
    switch (type->type) {
        BASIC_TYPES
    }
    return str_lit("Invalid");
}
#undef TYPE

static b8 C_CheckTypeEquals(P_Type* a, P_Type* b) {
    if (!a || !b) return false; 
    if (a->type != b->type) return false;
    switch (a->type) {
        // No extra Data associated with these types
        case BasicType_Integer:
        case BasicType_Void:
        case BasicType_Cstring: return true;
        
        case BasicType_Function: {
            if (!C_CheckTypeEquals(a->function.return_type, b->function.return_type)) return false;
            if (a->function.arity != b->function.arity) return false;
            for (u32 i = 0; i < a->function.arity; i++) {
                if (!C_CheckTypeEquals(a->function.param_types[i], b->function.param_types[i]))
                    return false;
            }
            return true;
        }
    }
    
    return false;
}

static b8 C_CheckBinary(P_Type* lhs, P_Type* rhs, L_TokenType op, P_Type** output) {
    C_BinaryOpBinding binding = binary_operator_bindings[op];
    for (u32 i = 0; i < binding.pairs_count; i++) {
        if (C_CheckTypeEquals(binding.pairs[i].a, lhs) && C_CheckTypeEquals(binding.pairs[i].b, rhs)) {
            *output = binding.pairs[i].ret;
            return true;
        }
    }
    return false;
}

static b8 C_CheckUnary(P_Type* operand, L_TokenType op, P_Type** output) {
    C_UnaryOpBinding binding = unary_operator_bindings[op];
    for (u32 i = 0; i < binding.count; i++) {
        if (C_CheckTypeEquals(binding.elems[i].a, operand)) {
            *output = binding.elems[i].ret;
            return true;
        }
    }
    return false;
}

//~ Checking
static b8 C_GetSymbol(C_Checker* checker, symbol_hash_table_key key_prototype, symbol_hash_table_value* val) {
    for (i32 i = checker->scope_depth; i >= 0; i--) {
        key_prototype.depth = (u32) i;
        if (symbol_hash_table_get(&checker->symbol_table, key_prototype, val))
            return true;
    }
    return false;
}

static C_ScopeContext* C_PushScope(C_Checker* checker, P_Type* new_fn_ret, b8 is_lambda_body) {
    checker->scope_depth++;
    C_ScopeContext* scope = malloc(sizeof(C_ScopeContext));
    memset(scope, 0, sizeof(C_ScopeContext));
    scope->upper = checker->current_scope_context;
    scope->function_return_type = checker->function_return_type;
    scope->is_in_func_body = checker->is_in_func_body;
    
    checker->current_scope_context = scope;
    checker->is_in_func_body = false;
    
    if (is_lambda_body) {
        checker->function_return_type = new_fn_ret;
        checker->is_in_func_body = true;
    }
    
    return scope;
}

static void C_PopScope(C_Checker* checker, C_ScopeContext* scope) {
    for (u32 k = 0; k < checker->symbol_table.cap; k++) {
        symbol_hash_table_entry e = checker->symbol_table.elems[k];
        if (!symbol_key_is_null(e.key)) {
            if (e.value.depth == checker->scope_depth) {
                symbol_hash_table_del(&checker->symbol_table, e.key);
            }
        }
    }
    
    checker->current_scope_context = scope->upper;
    checker->function_return_type = scope->function_return_type;
    checker->is_in_func_body = scope->is_in_func_body;
    
    free(checker->current_scope_context);
    
    checker->scope_depth--;
}

static void C_CheckInFunction(C_Checker* checker, AstNode* node) {
    if (memcmp(checker->function_return_type, &C_NullType, sizeof(P_Type)) == 0) {
        C_ReportCheckError(checker, node->id, "Cannot have statements outside of functions\n");
    }
}

static P_Type* C_GetType(C_Checker* checker, AstNode* node) {
    switch (node->type) {
        case NodeType_Ident: {
            symbol_hash_table_key key = (symbol_hash_table_key) { .name = node->Ident, .depth = checker->scope_depth };
            symbol_hash_table_value val;
            if (C_GetSymbol(checker, key, &val)) {
                if (val.type == SymbolType_Variable || val.type == SymbolType_Function) {
                    return val.variable_type;
                }
            } else C_ReportCheckError(checker, node->id, "Undefined Variable %.*s\n", str_expand(node->Ident));
            return &C_InvalidType;
        } break;
        
        case NodeType_IntLit: {
            return &C_IntegerType;
        } break;
        
        case NodeType_GlobalString: {
            return &C_CstringType;
        } break;
        
        case NodeType_Unary: {
            P_Type* xpr = C_GetType(checker, node->Unary.expr);
            P_Type* output;
            if (!C_CheckUnary(xpr, node->id.type, &output)) {
                C_ReportCheckError(checker, node->id, "Cannot apply unary operator %.*s to type %.*s\n", str_expand(L_GetTypeName(node->id.type)), str_expand(C_GetBasicTypeName(xpr)));
            }
            return output;
        } break;
        
        case NodeType_Binary: {
            P_Type* lhs = C_GetType(checker, node->Binary.left);
            P_Type* rhs = C_GetType(checker, node->Binary.right);
            P_Type* output;
            if (!C_CheckBinary(lhs, rhs, node->id.type, &output)) {
                C_ReportCheckError(checker, node->id, "Cannot apply binary operator %.*s to types %.*s and %.*s\n", str_expand(L_GetTypeName(node->id.type)), str_expand(C_GetBasicTypeName(lhs)), str_expand(C_GetBasicTypeName(rhs)));
            }
            return output;
        } break;
        
        case NodeType_Group: {
            return C_GetType(checker, node->Group);
        } break;
        
        case NodeType_Lambda: {
            C_ScopeContext* scope_context = C_PushScope(checker, node->Lambda.function_type->function.return_type, true);
            
            for (u32 i = 0; i < node->Lambda.function_type->function.arity; i++) {
                string var_name = node->Lambda.param_names[i];
                P_Type* type = node->Lambda.function_type->function.param_types[i];
                symbol_hash_table_key key = (symbol_hash_table_key) { .name = var_name, .depth = 0 };
                if (symbol_hash_table_get(&checker->symbol_table, key, nullptr)) {
                    C_ReportCheckError(checker, node->id, "Parameters have the same name %.*s\n", str_expand(var_name));
                }
                symbol_hash_table_set(&checker->symbol_table, key, (symbol_hash_table_value) { .type = SymbolType_Variable, .name = var_name, .depth = checker->scope_depth, .variable_type = type });
            }
            
            checker->no_scope = true;
            C_GetType(checker, node->Lambda.body);
            checker->no_scope = false;
            
            C_PopScope(checker, scope_context);
            return node->Lambda.function_type;
        } break;
        
        case NodeType_Call: {
            P_Type* callee_type = C_GetType(checker, node->Call.callee);
            if (callee_type->type != BasicType_Function) {
                C_ReportCheckError(checker, node->id, "Cannot call expression of type %.*s\n", str_expand(C_GetBasicTypeName(callee_type)));
                return &C_InvalidType;
            }
            
            if (callee_type->function.arity != node->Call.arity) {
                C_ReportCheckError(checker, node->id, "Wrong number of arguments passed to function %.*s. Expected %u got %u\n", str_expand(node->id.lexeme), callee_type->function.arity, node->Call.arity);
                return &C_InvalidType;
            }
            
            for (u32 i = 0; i < node->Call.arity; i++) {
                P_Type* curr_type = C_GetType(checker, node->Call.params[i]);
                if (!C_CheckTypeEquals(curr_type, callee_type->function.param_types[i])) {
                    C_ReportCheckError(checker, node->id, "Argument %u mismatched. Expected %.*s, got %.*s\n", i, str_expand(C_GetBasicTypeName(callee_type->function.param_types[i])), str_expand(C_GetBasicTypeName(curr_type)));
                    return &C_InvalidType;
                }
            }
            
            return callee_type->function.return_type;
        }
        
        case NodeType_Return: {
            C_CheckInFunction(checker, node);
            if (node->Return) {
                P_Type* returned = C_GetType(checker, node->Return);
                if (!C_CheckTypeEquals(returned, checker->function_return_type)) {
                    C_ReportCheckError(checker, node->id, "Return Type %.*s does not match with return type of function %.*s\n", str_expand(C_GetBasicTypeName(returned)), str_expand(C_GetBasicTypeName(checker->function_return_type)));
                }
            } else {
                if (!C_CheckTypeEquals(&C_VoidType, checker->function_return_type)) {
                    C_ReportCheckError(checker, node->id, "Return Type %.*s does not match with return type of function %.*s\n", str_expand(C_GetBasicTypeName(&C_VoidType)), str_expand(C_GetBasicTypeName(checker->function_return_type)));
                }
            }
            
            if (checker->is_in_func_body) checker->found_return = true;
            
            return &C_InvalidType;
        } break;
        
        case NodeType_ExprStatement: {
            C_GetType(checker, node->ExprStatement);
            return &C_InvalidType;
        } break;
        
        case NodeType_Block: {
            C_CheckInFunction(checker, node);
            C_ScopeContext* scope_ctx = C_PushScope(checker, nullptr, false);
            for (u32 i = 0; i < node->Block.count; i++) {
                AstNode* statement = node->Block.statements[i];
                C_GetType(checker, statement);
            }
            C_PopScope(checker, scope_ctx);
            
            return &C_InvalidType;
        } break;
        
        case NodeType_Assign: {
            C_CheckInFunction(checker, node);
            P_Type* symboltype = {0};
            
            symbol_hash_table_key key = (symbol_hash_table_key) { .name = node->id.lexeme, .depth = 0 };
            symbol_hash_table_value val;
            if (C_GetSymbol(checker, key, &val)) {
                if (val.type != SymbolType_Variable)
                    C_ReportCheckError(checker, node->id, "%.*s is not assignable\n", str_expand(node->id.lexeme));
                symboltype = val.variable_type;
            }
            
            P_Type* valuetype = C_GetType(checker, node->Assign.value);
            if (!C_CheckTypeEquals(symboltype, valuetype)) {
                C_ReportCheckError(checker, node->id, "Assignment type mismatch. got types %.*s and %.*s\n", str_expand(C_GetBasicTypeName(symboltype)), str_expand(C_GetBasicTypeName(valuetype)));
            }
            
            return &C_InvalidType;
        } break;
        
        case NodeType_VarDecl: {
            string var_name = node->id.lexeme;
            P_Type* type = node->VarDecl.type;
            
            if (node->VarDecl.value) {
                P_Type* valuetype = C_GetType(checker, node->VarDecl.value);
                if (!C_CheckTypeEquals(type, valuetype)) {
                    C_ReportCheckError(checker, node->id, "Assignment type mismatch. got types %.*s and %.*s\n", str_expand(C_GetBasicTypeName(type)), str_expand(C_GetBasicTypeName(valuetype)));
                }
                
                if (node->VarDecl.value->type == NodeType_Lambda) {
                    
                    symbol_hash_table_key key = (symbol_hash_table_key) { .name = var_name, .depth = 0 };
                    if (symbol_hash_table_get(&checker->symbol_table, key, nullptr)) {
                        C_ReportCheckError(checker, node->id, "Symbol %.*s already exists\n", str_expand(var_name));
                    }
                    symbol_hash_table_set(&checker->symbol_table, key, (symbol_hash_table_value) { .type = SymbolType_Function, .name = var_name, .depth = checker->scope_depth, .variable_type = type });
                    
                } else {
                    
                    symbol_hash_table_key key = (symbol_hash_table_key) { .name = var_name, .depth = 0 };
                    if (symbol_hash_table_get(&checker->symbol_table, key, nullptr)) {
                        C_ReportCheckError(checker, node->id, "Symbol %.*s already exists\n", str_expand(var_name));
                    }
                    symbol_hash_table_set(&checker->symbol_table, key, (symbol_hash_table_value) { .type = SymbolType_Variable, .name = var_name, .depth = checker->scope_depth, .variable_type = type });
                    
                }
            }
            
            return &C_InvalidType;
        } break;
    }
    return &C_InvalidType;
}

void C_Init(C_Checker* checker) {
    memset(checker, 0, sizeof(*checker));
    symbol_hash_table_init(&checker->symbol_table);
}

void C_CheckAst(C_Checker* checker, AstNode* node) {
    C_GetType(checker, node);
}

void C_Free(C_Checker* checker) {
    symbol_hash_table_free(&checker->symbol_table);
}
