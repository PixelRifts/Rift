/* date = February 11th 2022 7:37 am */

#ifndef PARSER_H
#define PARSER_H

#include "defines.h"
#include "lexer.h"
#include "mem.h"

/* Uses AST_NODE(Id, Name, Type) */
#define AST_NODES \
AST_NODE(Error, str_lit(""), i8)\
AST_NODE(EXPR_START, str_lit(""), i8)\
AST_NODE(Ident, str_lit("Identifier Expression"), string)\
AST_NODE(IntLit, str_lit("Integer Literal"), i64)\
AST_NODE(Binary, str_lit("Binary Expression"), struct {\
AstNode* left;\
AstNode* right;\
L_TokenType type;\
})\
AST_NODE(EXPR_END, str_lit(""), i8)\

#define AST_NODE(Id, Name, Type) NodeType_##Id,
typedef u32 P_NodeType;
enum {
    AST_NODES
};
#undef AST_NODE

typedef struct AstNode AstNode;
#define AST_NODE(Id, Name, Type) Type Id;
struct AstNode {
    P_NodeType type;
    union {
        AST_NODES
    };
};
#undef AST_NODE

typedef struct P_Parser {
    M_Pool node_pool;
    
    L_Lexer* lexer;
    
    L_Token prev;
    L_Token curr;
    L_Token next;
    
    b8 errored;
    u8 error_count;
} P_Parser;

void P_Init(P_Parser* parser, L_Lexer* lexer);
AstNode* P_Parse(P_Parser* parser);
void P_Free(P_Parser* parser);
void PrintAst(AstNode* node);

#endif //PARSER_H
