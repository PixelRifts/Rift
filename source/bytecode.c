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
        case ExprType_IntLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, expr->op.integer_lit);
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
                stack.values[stack.count++] = left + right;
            } break;
            case Opcode_Minus: {
                B_Value right = stack.values[--stack.count];
                B_Value left = stack.values[--stack.count];
                stack.values[stack.count++] = left - right;
            } break;
            case Opcode_Star: {
                B_Value right = stack.values[--stack.count];
                B_Value left = stack.values[--stack.count];
                stack.values[stack.count++] = left * right;
            } break;
            case Opcode_Slash: {
                B_Value right = stack.values[--stack.count];
                B_Value left = stack.values[--stack.count];
                stack.values[stack.count++] = left / right;
            } break;
            case Opcode_Percent: {
                B_Value left = stack.values[--stack.count];
                B_Value right = stack.values[--stack.count];
                stack.values[stack.count++] = left % right;
            } break;
            
        }
    }
    
    B_Value ret = stack.values[0];
    B_FreeValueArray(&stack);
    
    return ret;
}

static P_Expr* B_ValueToExpr(P_Parser* parser, B_Value result) {
    return P_MakeIntNode(parser, result);
}

P_Expr* B_EvaluateExpr(P_Parser* parser, P_Expr* expr) {
    B_Chunk chunk = {0};
    B_InitChunk(&chunk);
    B_WriteExprToChunk(&chunk, expr);
    B_Value result = B_RunChunk(&chunk);
    B_FreeChunk(&chunk);
    return B_ValueToExpr(parser, result);
}
