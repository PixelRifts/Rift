#include "vm.h"
#include <stdio.h>

DArray_Impl(u8);
Stack_Impl(VM_RuntimeValue);


// For completeness' sake
VM_Chunk VM_ChunkAlloc(void) {
	VM_Chunk chunk = {0};
	return chunk;
}

void VM_ChunkPushOp(VM_Chunk* chunk, u8 byte) {
	darray_add(u8, chunk, byte);
}

void VM_ChunkPushU32(VM_Chunk* chunk, u32 val) {
	darray_add_all(u8, chunk, (u8*)&val, sizeof(u32));
}

void VM_ChunkPush(VM_Chunk* chunk, u8* bytes, u32 count) {
	darray_add_all(u8, chunk, bytes, count);
}

void VM_ChunkFree(VM_Chunk* chunk) {
	darray_free(u8, chunk);
}

//~ VM Helpers

void VM_Lower(VM_Chunk* chunk, IR_Ast* ast) {
	switch (ast->type) {
		case AstType_IntLiteral: {
			VM_ChunkPushOp(chunk, Opcode_Push);
			VM_RuntimeValue value = {
				.type = RuntimeValueType_Integer,
				.as_int = ast->int_lit.value,
			};
			VM_ChunkPush(chunk, &value, sizeof(VM_RuntimeValue));
		} break;
		
		case AstType_FloatLiteral: {
			// TODO(voxel):
		} break;
		
		case AstType_ExprUnary: {
			VM_Lower(chunk, ast->unary.operand);
			VM_ChunkPushOp(chunk, Opcode_UnaryOp);
			VM_ChunkPushU32(chunk, ast->unary.operator.type);
		} break;
		
		case AstType_ExprBinary: {
			VM_Lower(chunk, ast->binary.a);
			VM_Lower(chunk, ast->binary.b);
			VM_ChunkPushOp(chunk, Opcode_BinaryOp);
			VM_ChunkPushU32(chunk, ast->binary.operator.type);
		} break;
		
		case AstType_StmtPrint: {
			VM_Lower(chunk, ast->print.value);
			VM_ChunkPushOp(chunk, Opcode_Print);
		} break;
		
		default: {} break;
	}
}

VM_Chunk VM_LowerConstexpr(IR_Ast* ast) {
	VM_Chunk chunk = VM_ChunkAlloc();
	VM_Lower(&chunk, ast);
	return chunk;
}


static VM_RuntimeValue VM_BinaryOp(VM_RuntimeValue v1, VM_RuntimeValue v2, L_TokenType op) {
	if (v1.type == RuntimeValueType_Integer && v2.type == RuntimeValueType_Integer) {
		switch (op) {
			case TokenType_Plus: v1.as_int = v1.as_int + v2.as_int; break;
			case TokenType_Minus: v1.as_int = v1.as_int - v2.as_int; break;
			case TokenType_Star: v1.as_int = v1.as_int * v2.as_int; break;
			case TokenType_Slash: v1.as_int = v1.as_int / v2.as_int; break;
			case TokenType_Percent: v1.as_int = v1.as_int % v2.as_int; break;
		}
	} else {
		// TODO(voxel): Error Invalid type pair
	}
	return v1;
}


static VM_RuntimeValue VM_UnaryOp(VM_RuntimeValue value, L_TokenType op) {
	if (value.type == RuntimeValueType_Integer) {
		if (op == TokenType_Plus) {
			// NO OP
		} else if (op == TokenType_Minus) {
			value.as_int = -value.as_int;
		} else {
			// TODO(voxel): Error Invalid operator
		}
	} else {
		// TODO(voxel): Error Invalid type
	}
	return value;
}

static void VM_Print(VM_RuntimeValue value) {
	switch (value.type) {
		case RuntimeValueType_Integer: {
			printf("Int32 %d\n", value.as_int); 
		} break;
		
		default:; // TODO(voxel): Error Invalid type
	}
}

#define PushValue() dstack_push(VM_RuntimeValue, &datastack, *(VM_RuntimeValue*)&chunk->elems[i])
#define ReadOp() (*(L_TokenType*)&chunk->elems[i])
VM_RuntimeValue VM_RunExprChunk(VM_Chunk* chunk) {
	dstack(VM_RuntimeValue) datastack = {0};
	
	u32 i = 0;
	while (i < chunk->len) {
		switch (chunk->elems[i]) {
			case Opcode_Nop: i++; break;
			case Opcode_Push: {
				i++;
				PushValue();
				i += sizeof(VM_RuntimeValue);
			} break;
			
			case Opcode_Pop: {
				i++;
				dstack_pop(VM_RuntimeValue, &datastack);
			} break;
			
			case Opcode_UnaryOp: {
				i++;
				L_TokenType op = ReadOp();
				i += sizeof(L_TokenType);
				VM_RuntimeValue v = dstack_pop(VM_RuntimeValue, &datastack);
				v = VM_UnaryOp(v, op);
				dstack_push(VM_RuntimeValue, &datastack, v);
			} break;
			
			case Opcode_BinaryOp: {
				i++;
				L_TokenType op = ReadOp();
				i += sizeof(L_TokenType);
				VM_RuntimeValue v2 = dstack_pop(VM_RuntimeValue, &datastack);
				VM_RuntimeValue v1 = dstack_pop(VM_RuntimeValue, &datastack);
				v1 = VM_BinaryOp(v1, v2, op);
				dstack_push(VM_RuntimeValue, &datastack, v1);
			} break;
			
			case Opcode_Print: {
				i++;
				VM_RuntimeValue value = dstack_pop(VM_RuntimeValue, &datastack);
				VM_Print(value);
			} break;
		}
	}
}
#undef PushValue