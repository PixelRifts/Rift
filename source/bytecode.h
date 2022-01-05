/* date = January 4th 2022 0:55 pm */

#ifndef BYTECODE_H
#define BYTECODE_H

#include "defines.h"
#include "parser.h"

#ifndef GROW_CAPACITY
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_CAPACITY_BIGGER(capacity) ((capacity) < 32 ? 32 : (capacity) * 2)
#endif

typedef u32 B_ValueKind;
enum {
    ValueKind_Char,
    ValueKind_Integer,
    ValueKind_Long,
    ValueKind_Float,
    ValueKind_Double,
    ValueKind_Bool,
};

typedef struct B_Value {
    B_ValueKind type;
    union {
        i8  char_val;
        i32 int_val;
        i64 long_val;
        f32 float_val;
        f64 double_val;
        b8  bool_val;
    };
} B_Value;

typedef struct B_ValueArray {
    u32 count;
    u32 capacity;
    B_Value* values;
} B_ValueArray;

void B_InitValueArray(B_ValueArray* array);
void B_WriteValueArray(B_ValueArray* array, B_Value value);
void B_FreeValueArray(B_ValueArray* array);

typedef u32 B_Opcode;
enum {
    Opcode_Push, Opcode_Pop,
    
    Opcode_Plus = 10,
    Opcode_Minus,
    Opcode_Star,
    Opcode_Slash,
    Opcode_Percent,
};

// Not allocated within an arena.
typedef struct B_Chunk {
    u32 count;
    u32 capacity;
    u8* code;
    
    B_ValueArray constants;
} B_Chunk;

void B_InitChunk(B_Chunk* chunk);
void B_WriteChunk(B_Chunk* chunk, u8 byte);
u8   B_AddConstant(B_Chunk* chunk, B_Value constant);
void B_FreeChunk(B_Chunk* chunk);

P_Expr* B_EvaluateExpr(P_Parser* parser, P_Expr* expr);

#endif //BYTECODE_H
