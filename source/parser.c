#include "parser.h"

#include <stdarg.h>

#include "base/log.h"

//~ Data

u32 infix_expr_precs[] = {
    [TokenType_Star] = Prec_Factor,
    [TokenType_Slash] = Prec_Factor,
    
    [TokenType_Plus] = Prec_Term,
    [TokenType_Minus] = Prec_Term,
    
    [TokenType_TokenTypeCount] = Prec_Invalid,
};

//~ Error Handling

static void ErrorHere(P_Parser* p, const char* error, ...) {
	printf("Parser error: ");
	
	va_list va;
	va_start(va, error);
	vprintf(error, va);
	va_end(va);
	
	printf("\n");
}

//~ Helpers

static void Advance(P_Parser* p) {
	p->prev = p->curr;
	p->curr = p->next;
	p->next = L_LexToken(p->lexer);
}

static void EatOrError(P_Parser* p, L_TokenType type) {
	if (p->next.type != type)
		ErrorHere(p, "Expected token %.*s but got %.*s", str_expand(L_GetTypeName(type)), str_expand(L_GetTypeName(p->next.type)));
	
	// Panic mode reset delimiters
	if (type == TokenType_Semicolon ||
		type == TokenType_CloseBrace ||
		type == TokenType_CloseParenthesis)
		p->panic_mode = false;
	Advance(p);
}

static b8 Match(P_Parser* p, L_TokenType type) {
	if (p->next.type == type) {
		Advance(p);
		return true;
	}
	return false;
}

//~ Ast Allocators

static IR_Ast* P_MakeIntLiteralNode(P_Parser* p, i32 value) {
	IR_Ast* ret = pool_alloc(p->ast_node_pool);
	ret->type = AstType_IntLiteral;
	ret->int_lit.value = value;
	return ret;
}

static IR_Ast* P_MakeFloatLiteralNode(P_Parser* p, f32 value) {
	IR_Ast* ret = pool_alloc(p->ast_node_pool);
	ret->type = AstType_FloatLiteral;
	ret->float_lit.value = value;
	return ret;
}

static IR_Ast* P_MakeExprUnaryNode(P_Parser* p, L_Token operator, IR_Ast* operand) {
	IR_Ast* ret = pool_alloc(p->ast_node_pool);
	ret->type = AstType_ExprUnary;
	ret->unary.operator = operator;
	ret->unary.operand = operand;
	return ret;
}

static IR_Ast* P_MakeExprBinaryNode(P_Parser* p, IR_Ast* a, L_Token operator, IR_Ast* b) {
	IR_Ast* ret = pool_alloc(p->ast_node_pool);
	ret->type = AstType_ExprBinary;
	ret->binary.operator = operator;
	ret->binary.a = a;
	ret->binary.b = b;
	return ret;
}


static IR_Ast* P_MakeStmtPrintNode(P_Parser* p, IR_Ast* value) {
	IR_Ast* ret = pool_alloc(p->ast_node_pool);
	ret->type = AstType_StmtPrint;
	ret->print.value = value;
	return ret;
}

//~ Parsing

IR_Ast* P_ParseExpression(P_Parser* p, P_Precedence prec);
IR_Ast* P_ParsePrefixExpression(P_Parser* p);

IR_Ast* P_ParsePrefixUnaryExpr(P_Parser* p) {
	Advance(p);
	L_Token operator = p->curr;
	return P_MakeExprUnaryNode(p, operator, P_ParsePrefixExpression(p));
}

IR_Ast* P_ParsePrefixExpression(P_Parser* p) {
	switch (p->curr.type) {
		case TokenType_IntLit: {
			Advance(p);
			i32 val = (i32) atoi((const char*)p->prev.lexeme.str);
			return P_MakeIntLiteralNode(p, val);
		} break;
		
		case TokenType_Plus:
		case TokenType_Minus: {
			return P_ParsePrefixUnaryExpr(p);
		} break;
		
		case TokenType_OpenParenthesis: {
            Advance(p);
            IR_Ast* in = P_ParseExpression(p, Prec_Invalid);
            EatOrError(p, TokenType_CloseParenthesis);
            return in;
        }
		
		default: {
			ErrorHere(p, "Unexpected token %.*s", str_expand(p->curr.lexeme)); 
		} break;
	}
	return 0;
}


IR_Ast* P_ParseInfixExpression(P_Parser* p, L_Token op, P_Precedence prec, IR_Ast* left) {
	switch (op.type) {
		case TokenType_Plus:
        case TokenType_Minus:
        case TokenType_Star:
        case TokenType_Slash:
        case TokenType_Percent: {
			IR_Ast* right = P_ParseExpression(p, prec);
			return P_MakeExprBinaryNode(p, left, op, right);
		} break;
		
		default: {
			ErrorHere(p, "Unexpected token %.*s", str_expand(p->curr.lexeme)); 
		} break;
	}
	return 0;
}

IR_Ast* P_ParseExpression(P_Parser* p, P_Precedence prec) {
	IR_Ast* lhs = P_ParsePrefixExpression(p);
	
	if (infix_expr_precs[p->curr.type] != Prec_Invalid) {
		Advance(p);
		L_Token op = p->prev;
		while (true) {
			if (infix_expr_precs[op.type] == Prec_Invalid) break;
			
			if (infix_expr_precs[op.type] >= prec) {
				lhs = P_ParseInfixExpression(p, op, infix_expr_precs[op.type] + 1, lhs);
				op = p->curr;
				
				if (infix_expr_precs[op.type] != Prec_Invalid) Advance(p);
			} else break;
		}
	}
	
	return lhs;
}

void DumpAST(IR_Ast* ast, u32 indent) {
	for (u32 i = 0; i < indent; i++) printf("\t");
	
	switch (ast->type) {
		case AstType_IntLiteral: {
			printf("Int lit: %d\n", ast->int_lit.value);
		} break;
		
		case AstType_ExprUnary: {
			printf("Unary expr: %.*s\n", str_expand(L_GetTypeName(ast->unary.operator.type)));
			DumpAST(ast->unary.operand, indent + 1);
		} break;
		
		case AstType_ExprBinary: {
			printf("Binary expr: %.*s\n", str_expand(L_GetTypeName(ast->binary.operator.type)));
			DumpAST(ast->binary.a, indent + 1);
			DumpAST(ast->binary.b, indent + 1);
		} break;
		
		default: break;
	}
}

IR_Ast* P_Parse(P_Parser* p) {
	IR_Ast* a = P_ParseExpression(p, Prec_Invalid);
	DumpAST(a, 0);
	return P_MakeStmtPrintNode(p, a);
}

//~ Lifecycle

void P_Init(P_Parser* p, L_Lexer* lexer) {
	MemoryZeroStruct(p, P_Parser);
	
	p->lexer = lexer;
	p->ast_node_pool = pool_make(sizeof(IR_Ast));
	
	Advance(p);
	Advance(p);
}

void P_Free(P_Parser* p) {
	pool_free(p->ast_node_pool);
}
