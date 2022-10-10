#include "checker.h"

//~ Type Cache
DArray_Impl(Type);

void TypeCache_Init(TypeCache* cache) {
	darray_reserve(Type, cache, TypeID_COUNT);
}

TypeID TypeCache_Register(TypeCache* cache, Type type) {
	IteratePtr(cache, i) {
		if (memcmp(&cache->elems[i], &type, sizeof(Type)) == 0) {
			return i;
		}
	}
	darray_add(Type, cache, type);
	return cache->len - 1;
}

// NOTE(voxel): @unsafe IF YOU DON'T KNOW WHAT YOU'RE DOING
// NOTE(voxel): ALWAYS USE TypeCache_Register
void TypeCache_RegisterWithID(TypeCache* cache, Type type, TypeID id) {
	cache->elems[id] = type;
}

Type* TypeCache_Get(TypeCache* cache, TypeID id) {
	return &cache->elems[id];
}

void TypeCache_Free(TypeCache* cache) {
	darray_free(Type, cache);
}

//~ Type Check

#include "tables.h"

static TypeID C_GetTypeAssociatedPair(TypePair* bindings, TypeID a) {
	for (u32 k = 0; ; k++) {
		if (bindings[k].a == TypeID_Invalid) break;
		if (bindings[k].a == a) {
			return bindings[k].r;
		}
	}
	return TypeID_Invalid;
}

static TypeID C_GetTypeAssociatedTriple(TypeTriple* bindings, TypeID a, TypeID b) {
	for (u32 k = 0; ; k++) {
		if (bindings[k].a == TypeID_Invalid) break;
		if (bindings[k].a == a && bindings[k].b == b) {
			return bindings[k].r;
		}
	}
	return TypeID_Invalid;
}


//~ Checker

static TypeID C_CheckUnaryOp(TypeID operand, L_TokenType op) {
	TypeID ret = 0;
	if (op == TokenType_Plus || op == TokenType_Minus) {
		ret = C_GetTypeAssociatedPair(unary_operator_table_plusminus, operand);
	} else {
		// TODO(voxel): Error
		ret = TypeID_Invalid;
	}
	return ret;
}

static TypeID C_CheckBinaryOp(TypeID a, TypeID b, L_TokenType op) {
	TypeID ret = 0;
	if (op == TokenType_Plus || op == TokenType_Minus) {
		ret = C_GetTypeAssociatedTriple(binary_operator_table_plusminus, a, b);
	} else if (op == TokenType_Star || op == TokenType_Slash) {
		ret = C_GetTypeAssociatedTriple(binary_operator_table_stardivmod, a, b);
	} else {
		// TODO(voxel): Error
		ret = TypeID_Invalid;
	}
	return ret;
}

static TypeID C_CheckAst(IR_Ast* ast) {
	switch (ast->type) {
		case AstType_IntLiteral: {
			return TypeID_Integer;
		} break;
		
		case AstType_FloatLiteral: {
			// TODO(voxel): 
			return TypeID_Invalid;
		} break;
		
		case AstType_ExprUnary: {
			TypeID operand_type = C_CheckAst(ast->unary.operand);
			return C_CheckUnaryOp(operand_type, ast->unary.operator.type);
		} break;
		
		case AstType_ExprBinary: {
			TypeID a_type = C_CheckAst(ast->binary.a);
			TypeID b_type = C_CheckAst(ast->binary.b);
			return C_CheckBinaryOp(a_type, b_type, ast->binary.operator.type);
		} break;
		
		case AstType_StmtPrint: return C_CheckAst(ast->print.value);
	}
	return TypeID_Invalid;
}

b8 C_Check(C_Checker* checker) {
	C_CheckAst(checker->ast);
	return !checker->errored;
}

void C_Init(C_Checker* checker, IR_Ast* ast) {
	MemoryZeroStruct(checker, C_Checker);
	checker->ast = ast;
	
	TypeCache_Init(&checker->type_cache);
	
	// NOTE(voxel): Register basic types so compiletime tables can be supah quick
	// NOTE(voxel): TypeCache_Init reserves the amount of space required
	// NOTE(voxel): so this unsafe code should be fine
	// NOTE(voxel): But remember to add name to TypeID_ enum before registering here
	TypeCache_RegisterWithID(&checker->type_cache, (Type) {
								 .kind = TypeKind_Invalid
							 }, TypeID_Invalid);
	TypeCache_RegisterWithID(&checker->type_cache, (Type) {
								 .kind = TypeKind_Regular,
								 .regular = RegularTypeKind_Integer
							 }, TypeID_Integer);
	
}

void C_Free(C_Checker* checker) {
	TypeCache_Free(&checker->type_cache);
}