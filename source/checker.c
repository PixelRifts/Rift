/*
 TODOS(voxel):
- Better Error Output. Should show line with ----^ underneath offending token
*/

#include "checker.h"
#include "checker_data.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

b8 var_key_is_null(var_hash_table_key k) { return k.name.size == 0 && k.depth == 0; }
b8 var_key_is_eq(var_hash_table_key a, var_hash_table_key b) { return str_eq(a.name, b.name) && a.depth == b.depth; }
u32 hash_var_key(var_hash_table_key k) { return str_hash(k.name) + k.depth; }
b8 var_val_is_null(var_hash_table_value v) { return v.type.type == BasicType_Invalid; }
b8 var_val_is_tombstone(var_hash_table_value v) { return v.type.type == BasicType_End; }
HashTable_Impl(var, var_key_is_null, var_key_is_eq, hash_var_key, (var_hash_table_value) { .type = (C_Type) { .type = BasicType_End } }, var_val_is_null, var_val_is_tombstone);

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

#define TYPE(Id, Name) case BasicType_##Id: return Name;
static string C_GetBasicTypeName(C_Type type) {
    switch (type.type) {
        BASIC_TYPES
    }
    return str_lit("Invalid");
}
#undef TYPE

static b8 C_CheckBinary(C_Type lhs, C_Type rhs, L_TokenType op) {
    C_BinaryOpBinding binding = binary_operator_bindings[op];
    for (u32 i = 0; i < binding.pairs_count; i++) {
        if (binding.pairs[i].a == lhs.type && binding.pairs[i].b == rhs.type)
            return true;
    }
    return false;
}

static b8 C_CheckUnary(C_Type operand, L_TokenType op) {
    C_UnaryOpBinding binding = unary_operator_bindings[op];
    for (u32 i = 0; i < binding.count; i++) {
        if (binding.elems[i] == operand.type)
            return true;
    }
    return false;
}

static C_Type C_GetType(C_Checker* checker, AstNode* node) {
    switch (node->type) {
        case NodeType_Ident: {
            var_hash_table_key key = (var_hash_table_key) { .name = { .str = node->VarDecl.name.start, .size = node->VarDecl.name.length }, .depth = 0 };
            var_hash_table_value val;
            if (var_hash_table_get(&checker->var_table, key, &val)) {
                return val.type;
            }
            return (C_Type) { .type = BasicType_Invalid };
        } break;
        
        case NodeType_IntLit: {
            return (C_Type) { .type = BasicType_Integer };
        } break;
        
        case NodeType_GlobalString: {
            return (C_Type) { .type = BasicType_Cstring };
        } break;
        
        case NodeType_Unary: {
            C_Type xpr = C_GetType(checker, node->Unary.expr);
            if (!C_CheckUnary(xpr, node->Unary.op.type)) {
                C_ReportCheckError(checker, node->Unary.op, "Cannot apply unary operator %.*s to type %.*s\n", str_expand(L_GetTypeName(node->Unary.op.type)), str_expand(C_GetBasicTypeName(xpr)));
            }
            return (C_Type) { .type = BasicType_Integer };
        } break;
        
        case NodeType_Binary: {
            C_Type lhs = C_GetType(checker, node->Binary.left);
            C_Type rhs = C_GetType(checker, node->Binary.right);
            if (!C_CheckBinary(lhs, rhs, node->Binary.op.type)) {
                C_ReportCheckError(checker, node->Binary.op, "Cannot apply binary operator %.*s to types %.*s and %.*s\n", str_expand(L_GetTypeName(node->Binary.op.type)), str_expand(C_GetBasicTypeName(lhs)), str_expand(C_GetBasicTypeName(rhs)));
            }
            return (C_Type) { .type = BasicType_Integer };
        } break;
        
        case NodeType_Return: {
            C_GetType(checker, node->Return);
            return (C_Type) { .type = BasicType_Invalid };
        } break;
        
        case NodeType_VarDecl: {
            var_hash_table_key key = (var_hash_table_key) { .name = { .str = node->VarDecl.name.start, .size = node->VarDecl.name.length }, .depth = 0 };
            if (var_hash_table_get(&checker->var_table, key, nullptr)) {
                C_ReportCheckError(checker, node->VarDecl.name, "Variable %.*s already exists\n", node->VarDecl.name.length, node->VarDecl.name.start);
            }
            var_hash_table_set(&checker->var_table, key, (var_hash_table_value) { .type = (C_Type) { .type = BasicType_Integer } });
            return (C_Type) { .type = BasicType_Invalid };
        } break;
    }
    return (C_Type) { .type = BasicType_Invalid };
}

void C_Init(C_Checker* checker) {
    var_hash_table_init(&checker->var_table);
}

void C_CheckAst(C_Checker* checker, AstNode* node) {
    C_GetType(checker, node);
}

void C_Free(C_Checker* checker) {
    var_hash_table_free(&checker->var_table);
}
