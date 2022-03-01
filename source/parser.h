/* date = February 11th 2022 7:37 am */

#ifndef PARSER_H
#define PARSER_H

#include "defines.h"
#include "lexer.h"
#include "mem.h"
#include "ds.h"

/* Uses AST_NODE(Id, Name, Type) */
#define AST_NODES \
AST_NODE(Error, str_lit(""), i8)\
AST_NODE(EXPR_START, str_lit(""), i8)\
AST_NODE(Ident, str_lit("Identifier Expression"), L_Token)\
AST_NODE(IntLit, str_lit("Integer Literal"), i64)\
AST_NODE(GlobalString, str_lit("Global String Literal"), struct {\
L_Token token;\
string value;\
})\
AST_NODE(Unary, str_lit("Unary Expression"), struct {\
AstNode* expr;\
L_Token  op;\
})\
AST_NODE(Binary, str_lit("Binary Expression"), struct {\
AstNode* left;\
AstNode* right;\
L_Token  op;\
})\
AST_NODE(Lambda, str_lit("Lambda Expression"), struct {\
P_Type* function_type;\
L_Token name;\
AstNode* body;\
})\
AST_NODE(EXPR_END, str_lit(""), i8)\
AST_NODE(STMT_START, str_lit(""), i8)\
AST_NODE(Return, str_lit("Return Statement"), AstNode*)\
AST_NODE(Block, str_lit("Block Statement"), struct {\
P_Scope scope;\
AstNode** statements;\
u32 count;\
})\
AST_NODE(Assign, str_lit("Variable Assignment"), struct {\
L_Token name;\
AstNode* value;\
})\
AST_NODE(VarDecl, str_lit("Variable Declaration"), struct {\
P_Type* type;\
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
    NodeType_Lambda,
    NodeType_EXPR_END,
    
    NodeType_STMT_START,
    NodeType_Return,
    NodeType_Block,
    NodeType_Assign,
    NodeType_VarDecl,
    NodeType_STMT_END,
};

typedef u32 P_BasicType;
enum {
    BasicType_Invalid,
    BasicType_Integer,
    BasicType_Void,
    BasicType_Function,
    BasicType_Cstring,
    BasicType_Count
};

typedef struct P_Type P_Type;
struct P_Type {
    P_BasicType type;
    L_Token token;
    
    union {
        struct { P_Type* return_type; P_Type** param_types; string* param_names; u32 arity; } function;
    };
};

typedef u32 P_ScopeType;
enum {
    ScopeType_Invalid,
    ScopeType_None,
    ScopeType_Count
};

typedef struct P_Scope {
    P_ScopeType type;
    // There will be more stuff here probably
} P_Scope;

#define AST_NODE(Id, Name, Type) Type Id;
typedef struct AstNode AstNode;
struct AstNode {
    P_NodeType type;
    union {
        AST_NODES
    };
};
#undef AST_NODE

typedef struct P_ParserSnap {
    L_Lexer lexer;
    
    L_Token prev;
    L_Token curr;
    L_Token next;
} P_ParserSnap;

struct P_Parser;

P_ParserSnap P_TakeSnapshot(struct P_Parser* parser);
void P_ApplySnapshot(struct P_Parser* parser, P_ParserSnap snap);

Array_Prototype(type_array, P_Type*);
Array_Prototype(node_array, AstNode*);

typedef struct P_Parser {
    M_Pool node_pool;
    M_Pool type_pool;
    M_Arena arena;
    
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

#endif //PARSER_H
