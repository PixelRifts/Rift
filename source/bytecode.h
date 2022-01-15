/* date = January 4th 2022 0:55 pm */

#ifndef BYTECODE_H
#define BYTECODE_H

#include "defines.h"
#include "str.h"
typedef struct P_Parser P_Parser;
typedef struct P_Expr P_Expr;

#ifndef GROW_CAPACITY
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_CAPACITY_BIGGER(capacity) ((capacity) < 32 ? 32 : (capacity) * 2)
#endif

// TODO(voxel): IMP: Make state structure for this module. For it's own

typedef u32 B_ObjKind;
enum {
    ObjKind_String,
};

typedef struct B_Obj {
    B_ObjKind type;
} B_Obj;

typedef struct B_ObjString {
    B_Obj obj;
    string str;
} B_ObjString;

#define OBJ_TYPE(value) (value.obj_val->type)

typedef u32 B_ValueKind;
enum {
    ValueKind_Null,
    ValueKind_Char,
    ValueKind_Integer,
    ValueKind_Long,
    ValueKind_Float,
    ValueKind_Double,
    ValueKind_Bool,
    ValueKind_Obj,
};

typedef struct B_Value {
    B_ValueKind type;
    union {
        i8     char_val;
        i32    int_val;
        i64    long_val;
        f32    float_val;
        f64    double_val;
        b8     bool_val;
        B_Obj* obj_val;
    };
} B_Value;

#define NULL_VAL          ((B_Value) { ValueKind_Null, .char_val = 0 })
#define CHAR_VAL(value)   ((B_Value) { ValueKind_Char, .char_val = value })
#define INT_VAL(value)    ((B_Value) { ValueKind_Integer, .int_val = value })
#define LONG_VAL(value)   ((B_Value) { ValueKind_Long, .long_val = value })
#define FLOAT_VAL(value)  ((B_Value) { ValueKind_Float, .float_val = value })
#define DOUBLE_VAL(value) ((B_Value) { ValueKind_Double, .double_val = value })
#define BOOL_VAL(value)   ((B_Value) { ValueKind_Bool, .bool_val = value })
#define OBJ_VAL(value)    ((B_Value) { ValueKind_Obj, .obj_val = value })

#define IS_NULL(value)   ((value).type == ValueKind_Null)
#define IS_CHAR(value)   ((value).type == ValueKind_Char)
#define IS_INT(value)    ((value).type == ValueKind_Integer)
#define IS_LONG(value)   ((value).type == ValueKind_Long)
#define IS_FLOAT(value)  ((value).type == ValueKind_Float)
#define IS_DOUBLE(value) ((value).type == ValueKind_Double)
#define IS_BOOL(value)   ((value).type == ValueKind_Bool)
#define IS_OBJ(value)    ((value).type == ValueKind_Obj)
#define IS_STRING(value) (IS_OBJ(value) && (value).obj_val->type == ObjKind_String)

#define AS_STRING(value) (((B_ObjString*)(value).obj_val)->str)

typedef struct B_ValueArray {
    u32 count;
    u32 capacity;
    B_Value* values;
} B_ValueArray;

void B_ValueArrayInit(B_ValueArray* array);
void B_ValueArrayWrite(B_ValueArray* array, B_Value value);
void B_ValueArrayFree(B_ValueArray* array);

typedef struct B_ValueStack {
    u32 capacity;
    u32 tos;
    B_Value* stack;
} B_ValueStack;

void B_ValueStackPush(B_ValueStack* stack, B_Value value);
void B_ValueStackPop(B_ValueStack* stack, B_Value* value);
void B_ValueStackPeek(B_ValueStack* stack, B_Value* value);
void B_ValueStackFree(B_ValueStack* stack);

typedef u32 B_Opcode;
enum {
    Opcode_Push, Opcode_Pop, Opcode_Variable,
    
    Opcode_Plus = 10, Opcode_Minus, Opcode_Star, Opcode_Slash,
    Opcode_Ampersand, Opcode_Pipe,
    Opcode_Tilde, Opcode_Bang,
    Opcode_BangEqual, Opcode_EqualEqual,
    Opcode_Less, Opcode_Greater,
    Opcode_LessEqual, Opcode_GreaterEqual,
    Opcode_AmpersandAmpersand, Opcode_PipePipe,
    Opcode_ShiftLeft, Opcode_ShiftRight,
    
    Opcode_Positive, Opcode_Negative, Opcode_Not, Opcode_Complement,
    Opcode_Preinc, Opcode_Predec
};

// Not allocated within an arena.
typedef struct B_Chunk {
    u32 count;
    u32 capacity;
    u8* code;
    
    B_ValueArray constants;
} B_Chunk;

typedef struct B_Interpreter {
    M_Arena arena;
} B_Interpreter;

void B_InitChunk(B_Chunk* chunk);
void B_WriteChunk(B_Chunk* chunk, u8 byte);
u8   B_AddConstant(B_Chunk* chunk, B_Value constant);
void B_FreeChunk(B_Chunk* chunk);

void B_SetVariable(B_Interpreter* interp, string name, P_Expr* value);
P_Expr* B_EvaluateExpr(B_Interpreter* interp, P_Expr* expr);

void B_Init(B_Interpreter* interp);
void B_Free(B_Interpreter* interp);

#endif //BYTECODE_H
