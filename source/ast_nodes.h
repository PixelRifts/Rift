/* date = October 5th 2022 5:54 pm */

#ifndef AST_NODES_H
#define AST_NODES_H

#include "defines.h"
#include "lexer.h"

//~ Ast node definitions

typedef struct IR_Ast IR_Ast;

typedef struct IR_AstExprIntLiteral {
	i32 value;
} IR_AstIntLiteral;

typedef struct IR_AstExprFloatLiteral {
	f32 value;
} IR_AstFloatLiteral;

typedef struct IR_AstExprUnary {
	L_Token operator;
	IR_Ast* operand;
} IR_AstExprUnary;

typedef struct IR_AstExprBinary {
	L_Token operator;
	IR_Ast* a;
	IR_Ast* b;
} IR_AstExprBinary;


typedef struct IR_AstStmtPrint {
	IR_Ast* value;
} IR_AstStmtPrint;

//~ Ast struct definition

typedef u32 IR_AstType;
enum {
	AstType_IntLiteral,
	AstType_FloatLiteral,
	AstType_ExprUnary,
	AstType_ExprBinary,
	
	AstType_StmtPrint,
	
	AstType_COUNT,
};

struct IR_Ast {
	IR_AstType type;
	
	union {
		IR_AstIntLiteral int_lit;
		IR_AstFloatLiteral float_lit;
		IR_AstExprUnary unary;
		IR_AstExprBinary binary;
		
		IR_AstStmtPrint print;
	};
};

#endif //AST_NODES_H
