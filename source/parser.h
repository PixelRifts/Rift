/* date = October 4th 2022 7:49 pm */

#ifndef PARSER_H
#define PARSER_H

#include "base/mem.h"
#include "lexer.h"

#include "ast_nodes.h"

typedef u32 P_Precedence;
enum {
	Prec_Invalid,
	
	Prec_Term,
	Prec_Factor,
	
	Prec_Max,
};

typedef struct P_Parser {
	L_Token prev;
	L_Token curr;
	L_Token next;
	
	L_Lexer* lexer;
	M_Pool* ast_node_pool;
	
	b8 panic_mode;
} P_Parser;


IR_Ast* P_ParseExpression(P_Parser* p, P_Precedence prec);
void P_Parse(P_Parser* p);

void P_Init(P_Parser* p, L_Lexer* lexer);
void P_Free(P_Parser* p);

#endif //PARSER_H
