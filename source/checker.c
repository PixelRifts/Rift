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
HashTable_Impl(symbol, symbol_key_is_null, symbol_key_is_eq, hash_symbol_key, { .type = SymbolType_Count }, symbol_val_is_null, symbol_val_is_tombstone);

//~ Diagnostics

static void C_Report(C_Checker* checker, L_Token token, const char* stage, const char* err, ...) {
    if (checker->errored) return;
    if (checker->error_count > 20) exit(-1);
    fprintf(stderr, "%.*s:%d:%d: ERROR: %s > ", str_expand(checker->filename), token.line, token.column, stage);
    va_list va;
    va_start(va, err);
    vfprintf(stderr, err, va);
    va_end(va);
    checker->errored = true;
}

#define C_ReportCheckError(checker, token, error, ...) C_Report(checker, token, "Checker", error, __VA_ARGS__)

//~ Type Stuff

#define TYPE(Id, Name, Cname) [BasicType_##Id] = Name,
string type_names[BasicType_Count + 1] = {
    BASIC_TYPES
};
#undef TYPE

static string C_GetBasicTypeName(P_Type* type) {
    if (!type || type->type >= BasicType_Count) return str_lit("Invalid");
    return type_names[type->type];
}

static b8 C_CheckTypeEquals(P_Type* a, P_Type* b) {
    if (!a || !b) return false; 
    if (a->type != b->type) return false;
    switch (a->type) {
        // No extra Data associated with these types
        case BasicType_Integer:
        case BasicType_Void:
        case BasicType_Boolean:
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
        
        case BasicType_Pointer: return C_CheckTypeEquals(a->pointer, b->pointer);
    }
    
    return false;
}

static b8 C_CheckBinary(C_Checker* checker, C_Value lhs, C_Value rhs, L_TokenType op, P_Type** output) {
    C_BinaryOpBinding binding = binary_operator_bindings[op];
    for (u32 i = 0; i < binding.pairs_count; i++) {
        if (C_CheckTypeEquals(binding.pairs[i].a, lhs.type)
            && C_CheckTypeEquals(binding.pairs[i].b, rhs.type)) {
            *output = binding.pairs[i].ret;
            return true;
        }
    }
    return false;
}

static P_Type* C_PushPointerType(C_Checker* checker, P_Type* pointee) {
    P_Type* pointer = arena_alloc(&checker->arena, sizeof(P_Type));
    pointer->type = BasicType_Pointer;
    pointer->token = (L_Token) {0};
    pointer->pointer = pointee;
    return pointer;
}

static b8 C_CheckUnary(C_Checker* checker, C_Value operand, L_TokenType op, P_Type** output) {
    if (op == TokenType_Star) {
        if (operand.type->type == BasicType_Pointer) {
            *output = operand.type->pointer;
            return true;
        } else return false;
    } else if (op == TokenType_Ampersand) {
        if ((operand.flags & ValueFlag_Assignable) != 0) {
            *output = C_PushPointerType(checker, operand.type);
            return true;
        }
    }
    
    C_UnaryOpBinding binding = unary_operator_bindings[op];
    for (u32 i = 0; i < binding.count; i++) {
        if (C_CheckTypeEquals(binding.elems[i].a, operand.type)) {
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

static C_ScopeContext* C_PushScope(C_Checker* checker, P_Scope* new_scope, P_Type* new_fn_ret, b8 is_lambda_body) {
    checker->scope_depth++;
    C_ScopeContext* scope = malloc(sizeof(C_ScopeContext));
    memset(scope, 0, sizeof(C_ScopeContext));
    scope->upper = checker->current_scope_context;
    scope->upper_scope = checker->current_scope;
    scope->function_return_type = checker->function_return_type;
    scope->is_in_func_body = checker->is_in_func_body;
    
    new_scope->parent = scope->upper_scope;
    new_scope->depth = checker->scope_depth;
    checker->current_scope_context = scope;
    checker->current_scope = new_scope;
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
    checker->current_scope = scope->upper_scope;
    
    free(scope);
    
    checker->scope_depth--;
}

static void C_CheckInFunction(C_Checker* checker, AstNode* node) {
    if (memcmp(checker->function_return_type, &C_NullType, sizeof(P_Type)) == 0) {
        C_ReportCheckError(checker, node->id, "Cannot have statements outside of functions\n", 0);
    }
}

static C_Value C_Check(C_Checker* checker, AstNode* node) {
    switch (node->type) {
        case NodeType_Ident: {
            symbol_hash_table_key key = (symbol_hash_table_key) { .name = node->Ident, .depth = checker->scope_depth };
            symbol_hash_table_value val;
            if (C_GetSymbol(checker, key, &val)) {
                if (val.type == SymbolType_Variable || val.type == SymbolType_Function) {
                    return val.variable;
                }
            } else C_ReportCheckError(checker, node->id, "Undefined Variable '%.*s'\n", str_expand(node->Ident));
            return (C_Value) { &C_InvalidType, 0 };
        } break;
        
        case NodeType_IntLit: {
            return (C_Value) { &C_IntegerType, 0 };
        } break;
        
        case NodeType_BoolLit: {
            return (C_Value) { &C_BooleanType, 0 };
        } break;
        
        case NodeType_GlobalString: {
            return (C_Value) { &C_CstringType, 0 };
        } break;
        
        case NodeType_Unary: {
            C_Value xpr = C_Check(checker, node->Unary.expr);
            P_Type* output;
            if (!C_CheckUnary(checker, xpr, node->id.type, &output)) {
                C_ReportCheckError(checker, node->id, "Cannot apply unary operator '%.*s' to type '%.*s'\n", str_expand(L_GetTypeName(node->id.type)), str_expand(C_GetBasicTypeName(xpr.type)));
            }
            u32 flags = 0;
            if (node->id.type == TokenType_Star) flags |= ValueFlag_Assignable;
            return (C_Value) { output, flags };
        } break;
        
        case NodeType_Binary: {
            C_Value lhs = C_Check(checker, node->Binary.left);
            C_Value rhs = C_Check(checker, node->Binary.right);
            P_Type* output;
            if (!C_CheckBinary(checker, lhs, rhs, node->id.type, &output)) {
                C_ReportCheckError(checker, node->id, "Cannot apply binary operator '%.*s' to types '%.*s' and '%.*s'\n", str_expand(L_GetTypeName(node->id.type)), str_expand(C_GetBasicTypeName(lhs.type)), str_expand(C_GetBasicTypeName(rhs.type)));
            }
            return (C_Value) { output, 0 };
        } break;
        
        case NodeType_Group: {
            return C_Check(checker, node->Group);
        } break;
        
        case NodeType_Lambda: {
            if (node->Lambda.is_native) { return (C_Value) { node->Lambda.function_type, 0 }; }
            
            C_ScopeContext* scope_context = C_PushScope(checker, node->Lambda.scope, node->Lambda.function_type->function.return_type, true);
            
            for (u32 i = 0; i < node->Lambda.function_type->function.arity; i++) {
                string var_name = node->Lambda.param_names[i];
                P_Type* type = node->Lambda.function_type->function.param_types[i];
                symbol_hash_table_key key = (symbol_hash_table_key) { .name = var_name, .depth = 0 };
                if (symbol_hash_table_get(&checker->symbol_table, key, nullptr)) {
                    C_ReportCheckError(checker, node->id, "Two parameters have the same name '%.*s' or parameter shadows another variable\n", str_expand(var_name));
                }
                symbol_hash_table_set(&checker->symbol_table, key, (symbol_hash_table_value) { .type = SymbolType_Variable, .name = var_name, .depth = checker->scope_depth, .variable = (C_Value) { type, ValueFlag_Assignable } });
            }
            
            checker->no_scope = true;
            C_Check(checker, node->Lambda.body);
            checker->no_scope = false;
            
            if (!checker->found_return && node->Lambda.function_type->function.return_type->type != BasicType_Void)
                C_ReportCheckError(checker, node->id, "Function '%.*s' does not return a value\n", str_expand(node->id.lexeme));
            checker->found_return = false;
            C_PopScope(checker, scope_context);
            return (C_Value) { node->Lambda.function_type, 0 };
        } break;
        
        case NodeType_Call: {
            C_Value callee_type = C_Check(checker, node->Call.callee);
            if (callee_type.type->type != BasicType_Function) {
                C_ReportCheckError(checker, node->id, "Cannot call expression of type '%.*s'\n", str_expand(C_GetBasicTypeName(callee_type.type)));
                return (C_Value) { &C_InvalidType, 0 };
            }
            
            b8 arity_check = callee_type.type->function.varargs
                ? callee_type.type->function.arity <= node->Call.arity : callee_type.type->function.arity == node->Call.arity;
            if (!arity_check) {
                C_ReportCheckError(checker, node->id, "Wrong number of arguments passed to function '%.*s'. Expected %u got %u\n", str_expand(node->id.lexeme), callee_type.type->function.arity, node->Call.arity);
                return (C_Value) { &C_InvalidType, 0 };
            }
            
            for (u32 i = 0; i < callee_type.type->function.arity; i++) {
                C_Value curr_type = C_Check(checker, node->Call.params[i]);
                if (!C_CheckTypeEquals(curr_type.type, callee_type.type->function.param_types[i])) {
                    C_ReportCheckError(checker, node->id, "Argument %u mismatched. Expected '%.*s', got '%.*s'\n", i,
                                       str_expand(C_GetBasicTypeName(callee_type.type->function.param_types[i])), str_expand(C_GetBasicTypeName(curr_type.type)));
                    return (C_Value) { &C_InvalidType, 0 };
                }
            }
            
            return (C_Value) { callee_type.type->function.return_type, 0 };
        }
        
        case NodeType_Return: {
            C_CheckInFunction(checker, node);
            if (node->Return) {
                C_Value returned = C_Check(checker, node->Return);
                if (!C_CheckTypeEquals(returned.type, checker->function_return_type)) {
                    C_ReportCheckError(checker, node->id, "Return Type '%.*s' does not match with return type of function '%.*s'\n", str_expand(C_GetBasicTypeName(returned.type)), str_expand(C_GetBasicTypeName(checker->function_return_type)));
                }
            } else {
                if (!C_CheckTypeEquals(&C_VoidType, checker->function_return_type)) {
                    C_ReportCheckError(checker, node->id, "Return Type '%.*s' does not match with return type of function '%.*s'\n", str_expand(C_GetBasicTypeName(&C_VoidType)), str_expand(C_GetBasicTypeName(checker->function_return_type)));
                }
            }
            
            if (checker->is_in_func_body) checker->found_return = true;
            
            return (C_Value) { &C_InvalidType, 0 };
        } break;
        
        case NodeType_ExprStatement: {
            C_Check(checker, node->ExprStatement);
            return (C_Value) { &C_InvalidType, 0 };
        } break;
        
        case NodeType_Block: {
            C_CheckInFunction(checker, node);
            C_ScopeContext* scope_ctx = nullptr;
            if (!checker->no_scope) scope_ctx = C_PushScope(checker, node->Block.scope, nullptr, false);
            for (u32 i = 0; i < node->Block.count; i++) {
                AstNode* statement = node->Block.statements[i];
                C_Check(checker, statement);
            }
            if (scope_ctx) C_PopScope(checker, scope_ctx);
            
            return (C_Value) { &C_InvalidType, 0 };
        } break;
        
        case NodeType_If: {
            C_CheckInFunction(checker, node);
            
            C_ScopeContext* scope_ctx = C_PushScope(checker, node->If.then_scope, nullptr, false);
            
            C_Value condition = C_Check(checker, node->If.condition);
            if (condition.type->type != BasicType_Boolean) {
                C_ReportCheckError(checker, node->id, "Condition for if statement is not a boolean\n", 0);
            }
            
            checker->no_scope = true;
            C_Check(checker, node->If.then);
            checker->no_scope = false;
            
            C_PopScope(checker, scope_ctx);
            
            if (node->If.elsee) {
                scope_ctx = C_PushScope(checker, node->If.else_scope, nullptr, false);
                checker->no_scope = true;
                C_Check(checker, node->If.elsee);
                checker->no_scope = false;
                C_PopScope(checker, scope_ctx);
            }
            
            return (C_Value) { &C_InvalidType, 0 };
        } break;
        
        case NodeType_While: {
            C_CheckInFunction(checker, node);
            C_ScopeContext* scope_ctx = C_PushScope(checker, node->While.scope, nullptr, false);
            C_Value condition = C_Check(checker, node->While.condition);
            if (condition.type->type != BasicType_Boolean) {
                C_ReportCheckError(checker, node->id, "Condition for While loop is not a boolean\n", 0);
            }
            
            checker->no_scope = true;
            C_Check(checker, node->While.body);
            checker->no_scope = false;
            
            C_PopScope(checker, scope_ctx);
            return (C_Value) { &C_InvalidType, 0 };
        } break;
        
        case NodeType_Assign: {
            C_CheckInFunction(checker, node);
            C_Value symboltype = {0};
            
            if (node->Assign.assignee->type == NodeType_Ident) {
                
                symbol_hash_table_key key = (symbol_hash_table_key) { .name = node->Assign.assignee->Ident, .depth = 0 };
                symbol_hash_table_value val;
                if (C_GetSymbol(checker, key, &val)) {
                    if ((val.variable.flags & ValueFlag_Assignable) == 0)
                        C_ReportCheckError(checker, node->id, "'%.*s' is not assignable\n", str_expand(node->Ident));
                    symboltype = val.variable;
                }
                
                C_Value valuetype = C_Check(checker, node->Assign.value);
                if (!C_CheckTypeEquals(symboltype.type, valuetype.type)) {
                    C_ReportCheckError(checker, node->id, "Assignment type mismatch. got types '%.*s' and '%.*s'\n", str_expand(C_GetBasicTypeName(symboltype.type)), str_expand(C_GetBasicTypeName(valuetype.type)));
                }
                
            } else if (node->Assign.assignee->type == NodeType_Unary && node->Assign.assignee->id.type == TokenType_Star) {
                // Deref
                C_Value assigneetype = C_Check(checker, node->Assign.assignee);
                C_Value valuetype = C_Check(checker, node->Assign.value);
                if (!C_CheckTypeEquals(assigneetype.type, valuetype.type)) {
                    C_ReportCheckError(checker, node->id, "Assignment type mismatch. got types '%.*s' and '%.*s'\n", str_expand(C_GetBasicTypeName(assigneetype.type->pointer)), str_expand(C_GetBasicTypeName(valuetype.type)));
                }
            } else {
                C_ReportCheckError(checker, node->Assign.assignee->id, "Cannot Assign to expression\n");
            }
            
            return (C_Value) { &C_InvalidType, 0 };
        } break;
        
        case NodeType_VarDecl: {
            string var_name = node->id.lexeme;
            P_Type* type = node->VarDecl.type;
            
            if (node->VarDecl.value) {
                C_Value valuetype = C_Check(checker, node->VarDecl.value);
                
                // Infer type
                if (type == nullptr) {
                    node->VarDecl.type = valuetype.type;
                    type = valuetype.type;
                }
                
                if (!C_CheckTypeEquals(type, valuetype.type)) {
                    C_ReportCheckError(checker, node->id, "Assignment type mismatch. got types '%.*s' and '%.*s'\n", str_expand(C_GetBasicTypeName(type)), str_expand(C_GetBasicTypeName(valuetype.type)));
                }
                
                if (node->VarDecl.value->type == NodeType_Lambda) {
                    
                    symbol_hash_table_key key = (symbol_hash_table_key) { .name = var_name, .depth = 0 };
                    if (symbol_hash_table_get(&checker->symbol_table, key, nullptr)) {
                        C_ReportCheckError(checker, node->id, "Symbol '%.*s' already exists\n", str_expand(var_name));
                    }
                    symbol_hash_table_set(&checker->symbol_table, key, (symbol_hash_table_value) { .type = SymbolType_Function, .name = var_name, .depth = checker->scope_depth, .variable = { type, 0 } });
                    
                } else {
                    
                    symbol_hash_table_key key = (symbol_hash_table_key) { .name = var_name, .depth = 0 };
                    if (symbol_hash_table_get(&checker->symbol_table, key, nullptr)) {
                        C_ReportCheckError(checker, node->id, "Symbol '%.*s' already exists\n", str_expand(var_name));
                    }
                    symbol_hash_table_set(&checker->symbol_table, key, (symbol_hash_table_value) { .type = SymbolType_Variable, .name = var_name, .depth = checker->scope_depth, .variable = { type, ValueFlag_Assignable } });
                }
            } else {
                if (type == nullptr) 
                    C_ReportCheckError(checker, node->id, "Variable '%.*s' has neither type nor value. At least one of these should be specified\n", str_expand(var_name));
                
                symbol_hash_table_key key = (symbol_hash_table_key) { .name = var_name, .depth = 0 };
                if (symbol_hash_table_get(&checker->symbol_table, key, nullptr)) {
                    C_ReportCheckError(checker, node->id, "Symbol '%.*s' already exists\n", str_expand(var_name));
                }
                symbol_hash_table_set(&checker->symbol_table, key, (symbol_hash_table_value) { .type = SymbolType_Variable, .name = var_name, .depth = checker->scope_depth, .variable = { type, ValueFlag_Assignable } });
            }
            
            return (C_Value) { &C_InvalidType, 0 };
        } break;
    }
    return (C_Value) { &C_InvalidType, 0 };
}

void C_Init(C_Checker* checker, string filename) {
    memset(checker, 0, sizeof(*checker));
    checker->filename = filename;
    symbol_hash_table_init(&checker->symbol_table);
    arena_init(&checker->arena);
}

void C_CheckAst(C_Checker* checker, AstNode* node) {
    C_Check(checker, node);
}

void C_Free(C_Checker* checker) {
    symbol_hash_table_free(&checker->symbol_table);
    arena_free(&checker->arena);
}
