#include "parser.h"

#include "base/log.h"

// TODO: Temporary
#define Error printf

static void Advance(P_Parser* p) {
	p->prev = p->curr;
	p->curr = p->next;
	p->next = L_LexToken(p->lexer);
}

static void EatOrError(P_Parser* p, L_TokenType type) {
	if (p->next.type != type)
		Error("Expected token %.*s but got %.*s\n", str_expand(L_GetTypeName(type)), str_expand(L_GetTypeName(p->next.type)));
	Advance(p);
}

static b8 Match(P_Parser* p, L_TokenType type) {
	if (p->next.type == type) {
		Advance(p);
		return true;
	}
	return false;
}

void P_Init(P_Parser* p, L_Lexer* lexer) {
	p->lexer = lexer;
	p->ast_node_pool = pool_make(sizeof(IR_Ast));
	
	Advance(p);
}

void P_Parse(P_Parser* p) {
	if (Match(p, TokenType_Bool)) {
		EatOrError(p, TokenType_Ident);
		EatOrError(p, TokenType_Equal);
		
		if (Match(p, TokenType_True))
			printf("TRUE!!\n");
		else if (Match(p, TokenType_False))
			printf("FALSE!!\n");
		
		Advance(p); // ;
	}
}

void P_Free(P_Parser* p) {
	pool_free(p->ast_node_pool);
}
