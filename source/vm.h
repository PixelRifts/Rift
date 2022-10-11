/* date = October 6th 2022 7:05 pm */

#ifndef VM_H
#define VM_H

#include "ast_nodes.h"
#include "base/ds.h"

//~ Chunk Helpers

DArray_Prototype(u8);

#if 0
typedef struct IR_Chunk IR_Chunk;
#endif
typedef darray(u8) IR_Chunk;

IR_Chunk IR_ChunkAlloc(void);
void IR_ChunkPushOp(IR_Chunk* chunk, u8 byte);
void IR_ChunkPushU32(IR_Chunk* chunk, u32 value);
void IR_ChunkPush(IR_Chunk* chunk, u8* bytes, u32 count);
void IR_ChunkFree(IR_Chunk* chunk);

//~ Opcodes and runtime values

typedef u32 VM_Opcode;
enum {
	Opcode_Nop,
	Opcode_Push,
	Opcode_Pop,
	Opcode_UnaryOp,
	Opcode_BinaryOp,
	Opcode_Print,
	
	Opcode_COUNT
};

typedef u32 VM_RuntimeValueType;
enum {
	RuntimeValueType_Invalid,
	RuntimeValueType_Integer,
	
	RuntimeValueType_COUNT,
};

typedef struct VM_RuntimeValue {
	VM_RuntimeValueType type;
	
	union {
		i32 as_int;
	};
} VM_RuntimeValue;

//~ VM Helpers

Stack_Prototype(VM_RuntimeValue);

void VM_Lower(IR_Chunk* chunk, IR_Ast* ast);
IR_Chunk VM_LowerConstexpr(IR_Ast* ast);

VM_RuntimeValue VM_RunExprChunk(IR_Chunk* chunk);

#endif //VM_H
