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
AST_NODE(GlobalString, str_lit("Global String Literal"), string)\
AST_NODE(Unary, str_lit("Unary Expression"), struct {\
AstNode* expr;\
L_Token  op;\
})\
AST_NODE(Binary, str_lit("Binary Expression"), struct {\
AstNode* left;\
AstNode* right;\
L_Token  op;\
})\
AST_NODE(EXPR_END, str_lit(""), i8)\
AST_NODE(STMT_START, str_lit(""), i8)\
AST_NODE(Return, str_lit("Return Statement"), AstNode*)\
AST_NODE(VarDecl, str_lit("Variable Decl Assignment Statement"), struct {\
L_Token type;\
L_Token name;\
AstNode* value;\
})\
AST_NODE(STMT_END, str_lit(""), i8)

typedef u32 P_NodeType;
enum {
    NodeType_Error,
    
    NodeType_EXPR_START,
    NodeType_Ident,
    NodeType_IntLit,
    NodeType_GlobalString,
    NodeType_Unary,
    NodeType_Binary,
    NodeType_EXPR_END,
    
    NodeType_STMT_START,
    NodeType_Return,
    NodeType_VarDecl,
    NodeType_STMT_END,
};

#define AST_NODE(Id, Name, Type) Type Id;
typedef struct AstNode AstNode;
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
