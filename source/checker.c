/*
 TODOS(voxel):
- Better Error Output. Should show line with ~~~~^ underneath offending token
*/

#include "checker.h"
#include "checker_data.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

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
static string C_GetBasicTypeName(C_BasicType type) {
    switch (type) {
        BASIC_TYPES
    }
    return str_lit("Invalid");
}
#undef TYPE

static b8 C_CheckBinary(C_BasicType lhs, C_BasicType rhs, L_TokenType op) {
    C_OperatorBinding binding = binary_operator_bindings[op];
    for (u32 i = 0; i < binding.pairs_count; i++) {
        if (binding.pairs[i].a == lhs && binding.pairs[i].b == rhs)
            return true;
    }
    return false;
}

static C_BasicType C_GetType(C_Checker* checker, AstNode* node) {
    switch (node->type) {
        case NodeType_Ident: {
            return BasicType_Invalid;
        } break;
        
        case NodeType_IntLit: {
            return BasicType_Integer;
        } break;
        
        case NodeType_Binary: {
            C_BasicType lhs = C_GetType(checker, node->Binary.left);
            C_BasicType rhs = C_GetType(checker, node->Binary.right);
            if (!C_CheckBinary(lhs, rhs, node->Binary.op.type)) {
                C_ReportCheckError(checker, node->Binary.op, "Cannot apply binary operator %.*s to types %.*s and %.*s\n", str_expand(L_GetTypeName(node->Binary.op.type)), str_expand(C_GetBasicTypeName(lhs)), str_expand(C_GetBasicTypeName(rhs)));
            }
            return BasicType_Integer;
        } break;
        
        case NodeType_Return: {
            C_BasicType t = C_GetType(checker, node->Return);
            if (t != BasicType_Integer) C_ReportCheckError(checker, node->Binary.op, "Cannot print expression of type %.*s\n", str_expand(C_GetBasicTypeName(t)));
            return BasicType_Invalid;
        } break;
    }
    return BasicType_Invalid;
}

void C_CheckAst(C_Checker* checker, AstNode* node) {
    C_GetType(checker, node);
}
