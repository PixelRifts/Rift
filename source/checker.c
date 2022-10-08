#include "checker.h"

//~ Type Check

static inline b8 Type_IsInteger(Type* type) {
	return (type->kind == TypeKind_Regular && type->regular == RegularTypeKind_Integer);
}

//~ Checker

void C_Init(C_Checker* checker, IR_Ast* ast) {
	MemoryZeroStruct(checker, C_Checker);
	checker->ast = ast;
}

// TODO(voxel): TODO(voxel): VERY IMP
// TODO(voxel): Maybe use some compiletime generated tables here

static Type C_CheckUnaryOp(Type* operand, L_TokenType op) {
	Type ret = {0};
	if (op == TokenType_Plus) {
		if (Type_IsInteger(operand)) {
			ret = *operand;
		}
	} else if (op == TokenType_Minus) {
		if (Type_IsInteger(operand)) {
			ret = *operand;
		}
	} else {
		// TODO(voxel): Error
	}
	return ret;
}

static Type C_CheckBinaryOp(Type* a, Type* b, L_TokenType op) {
	Type ret = {0};
	if (op == TokenType_Plus) {
		if (Type_IsInteger(a) && Type_IsInteger(b)) {
			ret = *a;
		}
	} else if (op == TokenType_Minus) {
		if (Type_IsInteger(a) && Type_IsInteger(b)) {
			ret = *b;
		}
	} else {
		// TODO(voxel): Error
	}
	return ret;
}

static Type C_CheckAst(IR_Ast* ast) {
	switch (ast->type) {
		case AstType_IntLiteral: {
			return (Type) {
				.kind = TypeKind_Regular,
				.regular = RegularTypeKind_Integer,
			};
		} break;
		
		case AstType_FloatLiteral: {
			// TODO(voxel): 
			return (Type) {0};
		} break;
		
		case AstType_ExprUnary: {
			Type operand_type = C_CheckAst(ast->unary.operand);
			return C_CheckUnaryOp(&operand_type, ast->unary.operator.type);
		} break;
		
		case AstType_ExprBinary: {
			Type a_type = C_CheckAst(ast->binary.a);
			Type b_type = C_CheckAst(ast->binary.b);
			return C_CheckBinaryOp(&a_type, &b_type, ast->binary.operator.type);
		} break;
		
		case AstType_StmtPrint: return C_CheckAst(ast->print.value);
	}
	return (Type) {0};
}

b8 C_Check(C_Checker* checker) {
	C_CheckAst(checker->ast);
	return !checker->errored;
}

void C_Free(C_Checker* checker) {
	// nop
}