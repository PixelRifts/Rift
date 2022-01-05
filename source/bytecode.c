#include "bytecode.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//~ Elpers
static P_Expr* P_MakeIntNode(P_Parser* parser, i32 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_IntLit;
    expr->ret_type = ValueType_Integer;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.integer_lit = value;
    return expr;
}

static P_Expr* P_MakeLongNode(P_Parser* parser, i64 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_LongLit;
    expr->ret_type = ValueType_Long;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.long_lit = value;
    return expr;
}

static P_Expr* P_MakeFloatNode(P_Parser* parser, f32 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_FloatLit;
    expr->ret_type = ValueType_Float;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.float_lit = value;
    return expr;
}

static P_Expr* P_MakeDoubleNode(P_Parser* parser, f64 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_DoubleLit;
    expr->ret_type = ValueType_Double;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.double_lit = value;
    return expr;
}

static P_Expr* P_MakeCharNode(P_Parser* parser, string value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_CharLit;
    expr->ret_type = ValueType_Char;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.char_lit = value;
    return expr;
}

static P_Expr* P_MakeBoolNode(P_Parser* parser, b8 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_BoolLit;
    expr->ret_type = ValueType_Bool;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.bool_lit = value;
    return expr;
}

//~ Values
void B_InitValueArray(B_ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void B_WriteValueArray(B_ValueArray* array, B_Value value) {
    if (array->capacity < array->count + 1) {
        B_Value* old_ptr = array->values;
        int old_cap = array->capacity;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values = calloc(array->capacity, sizeof(B_Value));
        memmove(array->values, old_ptr, old_cap);
        free(old_ptr);
    }
    
    array->values[array->count++] = value;
}

void B_FreeValueArray(B_ValueArray* array) {
    free(array->values);
}

//~ Chunks
void B_InitChunk(B_Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = nullptr;
    B_InitValueArray(&chunk->constants);
}

void B_WriteChunk(B_Chunk* chunk, u8 byte) {
    if (chunk->capacity < chunk->count + 1) {
        u8* old_ptr = chunk->code;
        int old_cap = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = calloc(chunk->capacity, sizeof(u8));
        memmove(chunk->code, old_ptr, old_cap);
        free(old_ptr);
    }
    
    chunk->code[chunk->count++] = byte;
}

u8 B_AddConstant(B_Chunk* chunk, B_Value constant) {
    B_WriteValueArray(&chunk->constants, constant);
    return chunk->constants.count - 1;
}

void B_FreeChunk(B_Chunk* chunk) {
    B_FreeValueArray(&chunk->constants);
    free(chunk->code);
}

//~ AST-To-Chunk
static void B_WriteExprToChunk(B_Chunk* chunk, P_Expr* expr) {
    switch (expr->type) {
        case ExprType_CharLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, (B_Value) { .type = ValueKind_Char, .char_val = (i8)expr->op.char_lit.str[1] });
            B_WriteChunk(chunk, loc);
        } break;
        case ExprType_IntLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, (B_Value) { .type = ValueKind_Integer, .int_val = expr->op.integer_lit });
            B_WriteChunk(chunk, loc);
        } break;
        case ExprType_LongLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, (B_Value) { .type = ValueKind_Long, .long_val = expr->op.long_lit });
            B_WriteChunk(chunk, loc);
        } break;
        case ExprType_FloatLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, (B_Value) { .type = ValueKind_Float, .float_val = expr->op.float_lit });
            B_WriteChunk(chunk, loc);
        } break;
        case ExprType_DoubleLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, (B_Value) { .type = ValueKind_Double, .double_val = expr->op.double_lit });
            B_WriteChunk(chunk, loc);
        } break;
        
        case ExprType_Binary: {
            B_WriteExprToChunk(chunk, expr->op.binary.left);
            B_WriteExprToChunk(chunk, expr->op.binary.right);
            B_WriteChunk(chunk, expr->op.binary.operator);
        } break;
        
        case ExprType_Unary: {
            B_WriteExprToChunk(chunk, expr->op.unary.operand);
            B_WriteChunk(chunk, expr->op.unary.operator);
        } break;
    }
}

static void B_ReadConstant(B_Chunk* chunk, B_ValueArray* stack, u32 i) {
    u8 loc = chunk->code[i];
    B_WriteValueArray(stack, chunk->constants.values[loc]);
}

//#define max(a, b) ((a) > (b)) ? a : b

#define B_ValueBinaryOp(a, b, op) \
do {\
B_ValueKind kind = max(a.type, b.type);\
switch(a.type) {\
case ValueKind_Char: {\
switch(b.type) {\
case ValueKind_Char: stack.values[stack.count++] = (B_Value) {.type = kind, .char_val = a.char_val op b.char_val}; break;\
case ValueKind_Integer: stack.values[stack.count++] = (B_Value) {.type = kind, .int_val = a.char_val op b.int_val}; break;\
case ValueKind_Long: stack.values[stack.count++] = (B_Value) {.type = kind, .long_val = a.char_val op b.long_val}; break;\
case ValueKind_Float: stack.values[stack.count++] = (B_Value) {.type = kind, .float_val = a.char_val op b.float_val}; break;\
case ValueKind_Double: stack.values[stack.count++] = (B_Value) {.type = kind, .double_val = a.char_val op b.double_val}; break;\
}\
} break;\
case ValueKind_Integer: {\
switch(b.type) {\
case ValueKind_Char: stack.values[stack.count++] = (B_Value) {.type = kind, .int_val = a.int_val op b.char_val}; break;\
case ValueKind_Integer: stack.values[stack.count++] = (B_Value) {.type = kind, .int_val = a.int_val op b.int_val}; break;\
case ValueKind_Long: stack.values[stack.count++] = (B_Value) {.type = kind, .long_val = a.int_val op b.long_val}; break;\
case ValueKind_Float: stack.values[stack.count++] = (B_Value) {.type = kind, .float_val = a.int_val op b.float_val}; break;\
case ValueKind_Double: stack.values[stack.count++] = (B_Value) {.type = kind, .double_val = a.int_val op b.double_val}; break;\
}\
} break;\
case ValueKind_Long: {\
switch(b.type) {\
case ValueKind_Char: stack.values[stack.count++] = (B_Value) {.type = kind, .long_val = a.long_val op b.char_val}; break;\
case ValueKind_Integer: stack.values[stack.count++] = (B_Value) {.type = kind, .long_val = a.long_val op b.int_val}; break;\
case ValueKind_Long: stack.values[stack.count++] = (B_Value) {.type = kind, .long_val = a.long_val op b.long_val}; break;\
case ValueKind_Float: stack.values[stack.count++] = (B_Value) {.type = kind, .float_val = a.long_val op b.float_val}; break;\
case ValueKind_Double: stack.values[stack.count++] = (B_Value) {.type = kind, .double_val = a.long_val op b.double_val}; break;\
}\
} break;\
case ValueKind_Float: {\
switch(b.type) {\
case ValueKind_Char: stack.values[stack.count++] = (B_Value) {.type = kind, .float_val = a.float_val op b.char_val}; break;\
case ValueKind_Integer: stack.values[stack.count++] = (B_Value) {.type = kind, .float_val = a.float_val op b.int_val}; break;\
case ValueKind_Long: stack.values[stack.count++] = (B_Value) {.type = kind, .float_val = a.float_val op b.long_val}; break;\
case ValueKind_Float: stack.values[stack.count++] = (B_Value) {.type = kind, .float_val = a.float_val op b.float_val}; break;\
case ValueKind_Double: stack.values[stack.count++] = (B_Value) {.type = kind, .double_val = a.float_val op b.double_val}; break;\
}\
} break;\
case ValueKind_Double: {\
switch(b.type) {\
case ValueKind_Char: stack.values[stack.count++] = (B_Value) {.type = kind, .double_val = a.double_val op b.char_val}; break;\
case ValueKind_Integer: stack.values[stack.count++] = (B_Value) {.type = kind, .double_val = a.double_val op b.int_val}; break;\
case ValueKind_Long: stack.values[stack.count++] = (B_Value) {.type = kind, .double_val = a.double_val op b.long_val}; break;\
case ValueKind_Float: stack.values[stack.count++] = (B_Value) {.type = kind, .double_val = a.double_val op b.float_val}; break;\
case ValueKind_Double: stack.values[stack.count++] = (B_Value) {.type = kind, .double_val = a.double_val op b.double_val}; break;\
}\
} break;\
}\
} while (0)\

static B_Value B_RunChunk(B_Chunk* chunk) {
    B_ValueArray stack = {0};
    B_InitValueArray(&stack);
    
    for (u32 i = 0; i < chunk->count;) {
        i++;
        switch (chunk->code[i - 1]) {
            case Opcode_Push: {
                B_ReadConstant(chunk, &stack, i++);
            } break;
            
            case Opcode_Pop: { stack.count--; } break;
            
            case Opcode_Plus: {
                B_Value right = stack.values[--stack.count];
                B_Value left = stack.values[--stack.count];
                B_ValueBinaryOp(left, right, +);
            } break;
            case Opcode_Minus: {
                B_Value right = stack.values[--stack.count];
                B_Value left = stack.values[--stack.count];
                B_ValueBinaryOp(left, right, -);
            } break;
            case Opcode_Star: {
                B_Value right = stack.values[--stack.count];
                B_Value left = stack.values[--stack.count];
                B_ValueBinaryOp(left, right, *);
            } break;
            case Opcode_Slash: {
                B_Value right = stack.values[--stack.count];
                B_Value left = stack.values[--stack.count];
                B_ValueBinaryOp(left, right, /);
            } break;
            case Opcode_Percent: {
                //B_Value left = stack.values[--stack.count];
                //B_Value right = stack.values[--stack.count];
                //B_ValueBinaryOp(left, right, %);
            } break;
            
        }
    }
    
    B_Value ret = stack.values[0];
    B_FreeValueArray(&stack);
    
    return ret;
}

static P_Expr* B_ValueToExpr(P_Parser* parser, B_Value result) {
    switch (result.type) {
        case ValueKind_Char: return P_MakeCharNode(parser, str_from_format(&parser->arena, "%c", result.char_val));
        case ValueKind_Integer: return P_MakeIntNode(parser, result.int_val);
        case ValueKind_Long: return P_MakeLongNode(parser, result.long_val);
        case ValueKind_Float: return P_MakeFloatNode(parser, result.float_val);
        case ValueKind_Double: return P_MakeDoubleNode(parser, result.double_val);
        case ValueKind_Bool: return P_MakeBoolNode(parser, result.bool_val);
    }
    return nullptr;
}

P_Expr* B_EvaluateExpr(P_Parser* parser, P_Expr* expr) {
    B_Chunk chunk = {0};
    B_InitChunk(&chunk);
    B_WriteExprToChunk(&chunk, expr);
    B_Value result = B_RunChunk(&chunk);
    B_FreeChunk(&chunk);
    return B_ValueToExpr(parser, result);
}
