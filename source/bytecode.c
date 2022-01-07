#include "bytecode.h"
#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static var_value_array vars = {0};

//~ Elpers
static P_Expr* P_MakeIntNode(B_Interpreter* interp, i32 value) {
    P_Expr* expr = arena_alloc(&interp->arena, sizeof(P_Expr));
    expr->type = ExprType_IntLit;
    expr->ret_type = ValueType_Integer;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.integer_lit = value;
    return expr;
}

static P_Expr* P_MakeLongNode(B_Interpreter* interp, i64 value) {
    P_Expr* expr = arena_alloc(&interp->arena, sizeof(P_Expr));
    expr->type = ExprType_LongLit;
    expr->ret_type = ValueType_Long;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.long_lit = value;
    return expr;
}

static P_Expr* P_MakeFloatNode(B_Interpreter* interp, f32 value) {
    P_Expr* expr = arena_alloc(&interp->arena, sizeof(P_Expr));
    expr->type = ExprType_FloatLit;
    expr->ret_type = ValueType_Float;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.float_lit = value;
    return expr;
}

static P_Expr* P_MakeDoubleNode(B_Interpreter* interp, f64 value) {
    P_Expr* expr = arena_alloc(&interp->arena, sizeof(P_Expr));
    expr->type = ExprType_DoubleLit;
    expr->ret_type = ValueType_Double;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.double_lit = value;
    return expr;
}

static P_Expr* P_MakeCharNode(B_Interpreter* interp, string value) {
    P_Expr* expr = arena_alloc(&interp->arena, sizeof(P_Expr));
    expr->type = ExprType_CharLit;
    expr->ret_type = ValueType_Char;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.char_lit = value;
    return expr;
}

static P_Expr* P_MakeBoolNode(B_Interpreter* interp, b8 value) {
    P_Expr* expr = arena_alloc(&interp->arena, sizeof(P_Expr));
    expr->type = ExprType_BoolLit;
    expr->ret_type = ValueType_Bool;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.bool_lit = value;
    return expr;
}

//~ Values
void B_ValueArrayInit(B_ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void B_ValueArrayWrite(B_ValueArray* array, B_Value value) {
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

void B_ValueArrayFree(B_ValueArray* array) {
    free(array->values);
}

void B_ValueStackPush(B_ValueStack* stack, B_Value ret) {
    if (stack->tos + 1 > stack->capacity) {
        void* prev = stack->stack;
        stack->capacity = GROW_CAPACITY_BIGGER(stack->capacity);
        stack->stack = calloc(stack->capacity, sizeof(B_Value));
        memmove(stack->stack, prev, stack->tos * sizeof(B_Value));
        free(prev);
    }
    *(stack->stack + stack->tos) = ret;
    stack->tos++;
}

void B_ValueStackPop(B_ValueStack* stack, B_Value* ret) {
    stack->tos--;
    if (ret != nullptr)
        *ret = stack->stack[stack->tos];
}

void B_ValueStackPeek(B_ValueStack* stack, B_Value* ret) {
    *ret = stack->stack[stack->tos - 1];
}

void B_ValueStackFree(B_ValueStack* stack) {
    free(stack->stack);
}

//~ Chunks
void B_InitChunk(B_Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = nullptr;
    B_ValueArrayInit(&chunk->constants);
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
    B_ValueArrayWrite(&chunk->constants, constant);
    return chunk->constants.count - 1;
}

void B_FreeChunk(B_Chunk* chunk) {
    B_ValueArrayFree(&chunk->constants);
    free(chunk->code);
}

//~ AST-To-Chunk
static void B_WriteExprToChunk(B_Interpreter* interp, B_Chunk* chunk, P_Expr* expr) {
    switch (expr->type) {
        case ExprType_CharLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, CHAR_VAL((i8)expr->op.char_lit.str[1]));
            B_WriteChunk(chunk, loc);
        } break;
        case ExprType_IntLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, INT_VAL(expr->op.integer_lit));
            B_WriteChunk(chunk, loc);
        } break;
        case ExprType_LongLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, LONG_VAL(expr->op.long_lit));
            B_WriteChunk(chunk, loc);
        } break;
        case ExprType_FloatLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, FLOAT_VAL(expr->op.float_lit));
            B_WriteChunk(chunk, loc);
        } break;
        case ExprType_DoubleLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, DOUBLE_VAL(expr->op.double_lit));
            B_WriteChunk(chunk, loc);
        } break;
        case ExprType_BoolLit: {
            B_WriteChunk(chunk, Opcode_Push);
            u8 loc = B_AddConstant(chunk, BOOL_VAL(expr->op.bool_lit));
            B_WriteChunk(chunk, loc);
        } break;
        
        case ExprType_Binary: {
            B_WriteExprToChunk(interp, chunk, expr->op.binary.left);
            B_WriteExprToChunk(interp, chunk, expr->op.binary.right);
            B_WriteChunk(chunk, expr->op.binary.operator);
        } break;
        
        case ExprType_Unary: {
            B_WriteExprToChunk(interp, chunk, expr->op.unary.operand);
            B_WriteChunk(chunk, expr->op.unary.operator);
        } break;
        
        case ExprType_Variable: {
            B_ObjString* str = arena_alloc(&interp->arena, sizeof(B_ObjString));
            str->obj.type = ObjKind_String;
            str->str = expr->op.variable;
            u8 loc = B_AddConstant(chunk, OBJ_VAL((B_Obj*)((void*)str)));
            B_WriteChunk(chunk, Opcode_Variable);
            B_WriteChunk(chunk, loc);
        } break;
    }
}

static void B_ReadConstant(B_Chunk* chunk, B_ValueStack* stack, u32 i) {
    u8 loc = chunk->code[i];
    B_ValueStackPush(stack, chunk->constants.values[loc]);
}

#define B_ValueUnaryOp_NoFloat(a, op) \
do {\
switch (a.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, CHAR_VAL(op a.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, INT_VAL(op a.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, LONG_VAL(op a.long_val)); break;\
}\
} while (0)
#define B_ValueUnaryOp(a, op) \
do {\
switch (a.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, CHAR_VAL(op a.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, INT_VAL(op a.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, LONG_VAL(op a.long_val)); break;\
case ValueKind_Float: B_ValueStackPush(&stack, FLOAT_VAL(op a.float_val)); break;\
case ValueKind_Double: B_ValueStackPush(&stack, DOUBLE_VAL(op a.double_val)); break;\
}\
} while (0)
#define B_ValueBinaryOp(a, b, op) \
do {\
switch(a.type) {\
case ValueKind_Char: {\
switch(b.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, CHAR_VAL(a.char_val op b.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, INT_VAL(a.char_val op b.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, LONG_VAL(a.char_val op b.long_val)); break;\
case ValueKind_Float: B_ValueStackPush(&stack, FLOAT_VAL(a.char_val op b.float_val)); break;\
case ValueKind_Double: B_ValueStackPush(&stack, DOUBLE_VAL(a.char_val op b.double_val)); break;\
}\
} break;\
case ValueKind_Integer: {\
switch(b.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, INT_VAL(a.int_val op b.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, INT_VAL(a.int_val op b.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, LONG_VAL(a.int_val op b.long_val)); break;\
case ValueKind_Float: B_ValueStackPush(&stack, FLOAT_VAL(a.int_val op b.float_val)); break;\
case ValueKind_Double: B_ValueStackPush(&stack, DOUBLE_VAL(a.int_val op b.double_val)); break;\
}\
} break;\
case ValueKind_Long: {\
switch(b.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, LONG_VAL(a.long_val op b.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, LONG_VAL(a.long_val op b.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, LONG_VAL(a.long_val op b.long_val)); break;\
case ValueKind_Float: B_ValueStackPush(&stack, FLOAT_VAL(a.long_val op b.float_val)); break;\
case ValueKind_Double: B_ValueStackPush(&stack, DOUBLE_VAL(a.long_val op b.double_val)); break;\
}\
} break;\
case ValueKind_Float: {\
switch(b.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, FLOAT_VAL(a.float_val op b.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, FLOAT_VAL(a.float_val op b.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, FLOAT_VAL(a.float_val op b.long_val)); break;\
case ValueKind_Float: B_ValueStackPush(&stack, FLOAT_VAL(a.float_val op b.float_val)); break;\
case ValueKind_Double: B_ValueStackPush(&stack, DOUBLE_VAL(a.float_val op b.double_val)); break;\
}\
} break;\
case ValueKind_Double: {\
switch(b.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, DOUBLE_VAL(a.double_val op b.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, DOUBLE_VAL(a.double_val op b.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, DOUBLE_VAL(a.double_val op b.long_val)); break;\
case ValueKind_Float: B_ValueStackPush(&stack, DOUBLE_VAL(a.double_val op b.float_val)); break;\
case ValueKind_Double: B_ValueStackPush(&stack, DOUBLE_VAL(a.double_val op b.double_val)); break;\
}\
} break;\
}\
} while (0)
#define B_ValueBinaryOp_NoFloat(a, b, op) \
do {\
switch(a.type) {\
case ValueKind_Char: {\
switch(b.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, CHAR_VAL(a.char_val op b.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, INT_VAL(a.char_val op b.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, LONG_VAL(a.char_val op b.long_val)); break;\
}\
} break;\
case ValueKind_Integer: {\
switch(b.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, INT_VAL(a.int_val op b.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, INT_VAL(a.int_val op b.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, LONG_VAL(a.int_val op b.long_val)); break;\
}\
} break;\
case ValueKind_Long: {\
switch(b.type) {\
case ValueKind_Char: B_ValueStackPush(&stack, LONG_VAL(a.long_val op b.char_val)); break;\
case ValueKind_Integer: B_ValueStackPush(&stack, LONG_VAL(a.long_val op b.int_val)); break;\
case ValueKind_Long: B_ValueStackPush(&stack, LONG_VAL(a.long_val op b.long_val)); break;\
}\
} break;\
}\
} while (0)

static B_Value B_RunChunk(B_Chunk* chunk) {
    B_ValueStack stack = {0};
    
    for (u32 i = 0; i < chunk->count;) {
        i++;
        switch (chunk->code[i - 1]) {
            case Opcode_Push: { B_ReadConstant(chunk, &stack, i++); } break;
            case Opcode_Pop: { B_ValueStackPop(&stack, nullptr); } break;
            case Opcode_Variable: {
                string name = AS_STRING(chunk->constants.values[chunk->code[i++]]);
                for (u32 i = 0; i < vars.count; i++) {
                    if (str_eq(vars.elements[i].name, name)) {
                        B_ValueStackPush(&stack, vars.elements[i].value);
                    }
                }
            } break;
            
            case Opcode_Plus: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp(left, right, +);
            } break;
            case Opcode_Minus: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp(left, right, -);
            } break;
            case Opcode_Star: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp(left, right, *);
            } break;
            case Opcode_Slash: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp(left, right, /);
            } break;
            case Opcode_Ampersand: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp_NoFloat(left, right, &);
            } break;
            case Opcode_Pipe: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp_NoFloat(left, right, |);
            } break;
            
            case Opcode_ShiftLeft: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp_NoFloat(left, right, <<);
            } break;
            case Opcode_ShiftRight: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp_NoFloat(left, right, >>);
            } break;
            
            case Opcode_Less: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp(left, right, <);
                stack.stack[stack.tos - 1].type = ValueKind_Bool;
            } break;
            case Opcode_LessEqual: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp(left, right, <=);
                stack.stack[stack.tos - 1].type = ValueKind_Bool;
            } break;
            case Opcode_Greater: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp(left, right, >);
                stack.stack[stack.tos - 1].type = ValueKind_Bool;
            } break;
            case Opcode_GreaterEqual: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueBinaryOp(left, right, >=);
                stack.stack[stack.tos - 1].type = ValueKind_Bool;
            } break;
            
            case Opcode_EqualEqual: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                if (!IS_BOOL(left) && !IS_BOOL(right)) {
                    B_ValueBinaryOp(left, right, ==);
                    stack.stack[stack.tos - 1].type = ValueKind_Bool;
                } else B_ValueStackPush(&stack, BOOL_VAL(left.bool_val == right.bool_val));
            } break;
            
            case Opcode_BangEqual: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                if (!IS_BOOL(left) && !IS_BOOL(right)) {
                    B_ValueBinaryOp(left, right, !=);
                    stack.stack[stack.tos - 1].type = ValueKind_Bool;
                } else B_ValueStackPush(&stack, BOOL_VAL(left.bool_val != right.bool_val));
            } break;
            
            case Opcode_AmpersandAmpersand: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueStackPush(&stack, BOOL_VAL(left.bool_val && right.bool_val));
            } break;
            
            case Opcode_PipePipe: {
                B_Value right, left;
                B_ValueStackPop(&stack, &right);
                B_ValueStackPop(&stack, &left);
                B_ValueStackPush(&stack, BOOL_VAL(left.bool_val || right.bool_val));
            } break;
            
            case Opcode_Tilde: {
                B_Value a;
                B_ValueStackPop(&stack, &a);
                B_ValueUnaryOp_NoFloat(a, ~);
            } break;
            
            case Opcode_Bang: {
                B_Value a;
                B_ValueStackPop(&stack, &a);
                B_ValueStackPush(&stack, BOOL_VAL(!a.bool_val));
            } break;
        }
    }
    
    B_Value ret = stack.stack[0];
    B_ValueStackFree(&stack);
    
    return ret;
}

static P_Expr* B_ValueToExpr(B_Interpreter* interp, B_Value result) {
    switch (result.type) {
        case ValueKind_Char: return P_MakeCharNode(interp, str_from_format(&interp->arena, "%c", result.char_val));
        case ValueKind_Integer: return P_MakeIntNode(interp, result.int_val);
        case ValueKind_Long: return P_MakeLongNode(interp, result.long_val);
        case ValueKind_Float: return P_MakeFloatNode(interp, result.float_val);
        case ValueKind_Double: return P_MakeDoubleNode(interp, result.double_val);
        case ValueKind_Bool: return P_MakeBoolNode(interp, result.bool_val);
    }
    return nullptr;
}

void B_SetVariable(B_Interpreter* interp, string name, P_Expr* value) {
    B_Chunk chunk = {0};
    B_InitChunk(&chunk);
    B_WriteExprToChunk(interp, &chunk, value);
    B_Value resolved = B_RunChunk(&chunk);
    B_FreeChunk(&chunk);
    
    var_value_array_add(&interp->arena, &vars, (var_value_entry) { .name = name, .value = resolved });
}

P_Expr* B_EvaluateExpr(B_Interpreter* interp, P_Expr* expr) {
    B_Chunk chunk = {0};
    B_InitChunk(&chunk);
    B_WriteExprToChunk(interp, &chunk, expr);
    B_Value result = B_RunChunk(&chunk);
    B_FreeChunk(&chunk);
    return B_ValueToExpr(interp, result);
}

void B_Init(B_Interpreter* interp) {
    arena_init(&interp->arena);
}

void B_Free(B_Interpreter* interp) {
    arena_free(&interp->arena);
}
