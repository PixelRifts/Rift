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

static void C_PushScope(C_Checker* checker) {
    checker->scope_depth++;
}

static void C_PopScope(C_Checker* checker) {
    for (u32 k = 0; k < checker->symbol_table.cap; k++) {
        symbol_hash_table_entry e = checker->symbol_table.elems[k];
        if (!symbol_key_is_null(e.key)) {
            if (e.value.depth == checker->scope_depth) {
                symbol_hash_table_del(&checker->symbol_table, e.key);
            }
        }
    }
    checker->scope_depth--;
}

static P_Type* C_GetType(C_Checker* checker, AstNode* node) {
    switch (node->type) {
        case NodeType_Ident: {
            symbol_hash_table_key key = (symbol_hash_table_key) { .name = node->Ident.lexeme, .depth = checker->scope_depth };
            symbol_hash_table_value val;
            if (C_GetSymbol(checker, key, &val)) {
                if (val.type == SymbolType_Variable) {
                    return val.variable_type;
                }
            } else C_ReportCheckError(checker, node->Ident, "Undefined Variable %.*s\n", str_expand(node->Ident.lexeme));
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
            if (!C_CheckUnary(xpr, node->Unary.op.type, &output)) {
                C_ReportCheckError(checker, node->Unary.op, "Cannot apply unary operator %.*s to type %.*s\n", str_expand(L_GetTypeName(node->Unary.op.type)), str_expand(C_GetBasicTypeName(xpr)));
            }
            return output;
        } break;
        
        case NodeType_Binary: {
            P_Type* lhs = C_GetType(checker, node->Binary.left);
            P_Type* rhs = C_GetType(checker, node->Binary.right);
            P_Type* output;
            if (!C_CheckBinary(lhs, rhs, node->Binary.op.type, &output)) {
                C_ReportCheckError(checker, node->Binary.op, "Cannot apply binary operator %.*s to types %.*s and %.*s\n", str_expand(L_GetTypeName(node->Binary.op.type)), str_expand(C_GetBasicTypeName(lhs)), str_expand(C_GetBasicTypeName(rhs)));
            }
            return output;
        } break;
        
        case NodeType_Return: {
            C_GetType(checker, node->Return);
            return &C_InvalidType;
        } break;
        
        case NodeType_Block: {
            C_PushScope(checker);
            for (u32 i = 0; i < node->Block.count; i++) {
                AstNode* statement = node->Block.statements[i];
                C_GetType(checker, statement);
            }
            C_PopScope(checker);
            
            return &C_InvalidType;
        } break;
        
        case NodeType_Assign: {
            P_Type* symboltype = {0};
            
            symbol_hash_table_key key = (symbol_hash_table_key) { .name = node->Assign.name.lexeme, .depth = 0 };
            symbol_hash_table_value val;
            if (C_GetSymbol(checker, key, &val)) {
                if (val.type != SymbolType_Variable)
                    C_ReportCheckError(checker, node->Assign.name, "%.*s is not assignable\n", str_expand(node->Assign.name.lexeme));
                symboltype = val.variable_type;
            }
            
            P_Type* valuetype = C_GetType(checker, node->Assign.value);
            if (!C_CheckTypeEquals(symboltype, valuetype)) {
                C_ReportCheckError(checker, node->Assign.name, "Assignment type mismatch. got types %.*s and %.*s\n", str_expand(C_GetBasicTypeName(symboltype)), str_expand(C_GetBasicTypeName(valuetype)));
            }
            
            return &C_InvalidType;
        } break;
        
        case NodeType_VarDecl: {
            string var_name = node->VarDecl.name.lexeme;
            P_Type* type = node->VarDecl.type;
            
            P_Type* valuetype = C_GetType(checker, node->VarDecl.value);
            if (!C_CheckTypeEquals(type, valuetype)) {
                C_ReportCheckError(checker, node->Assign.name, "Assignment type mismatch. got types %.*s and %.*s\n", str_expand(C_GetBasicTypeName(type)), str_expand(C_GetBasicTypeName(valuetype)));
            }
            symbol_hash_table_key key = (symbol_hash_table_key) { .name = var_name, .depth = 0 };
            if (symbol_hash_table_get(&checker->symbol_table, key, nullptr)) {
                C_ReportCheckError(checker, node->VarDecl.name, "Variable %.*s already exists\n", str_expand(var_name));
            }
            symbol_hash_table_set(&checker->symbol_table, key, (symbol_hash_table_value) { .type = SymbolType_Variable, .name = var_name, .depth = checker->scope_depth, .variable_type = type });
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
