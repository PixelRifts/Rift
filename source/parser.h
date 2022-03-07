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
AST_NODE(Ident, str_lit("Identifier Expression"), string)\
AST_NODE(IntLit, str_lit("Integer Literal"), i64)\
AST_NODE(BoolLit, str_lit("Boolean Literal"), b8)\
AST_NODE(GlobalString, str_lit("Global String Literal"), struct {\
string value;\
})\
AST_NODE(Unary, str_lit("Unary Expression"), struct {\
AstNode* expr;\
})\
AST_NODE(Binary, str_lit("Binary Expression"), struct {\
AstNode* left;\
AstNode* right;\
})\
AST_NODE(Group, str_lit("Group Expression"), AstNode*)\
AST_NODE(Lambda, str_lit("Lambda Expression"), struct {\
P_Scope scope;\
P_Type* function_type;\
string* param_names;\
AstNode* body;\
})\
AST_NODE(Call, str_lit("Call Expression"), struct {\
AstNode* callee;\
AstNode** params;\
u32 arity;\
})\
AST_NODE(EXPR_END, str_lit(""), i8)\
AST_NODE(STMT_START, str_lit(""), i8)\
AST_NODE(Return, str_lit("Return Statement"), AstNode*)\
AST_NODE(ExprStatement, str_lit("Return Statement"), AstNode*)\
AST_NODE(Block, str_lit("Block Statement"), struct {\
P_Scope scope;\
AstNode** statements;\
u32 count;\
})\
AST_NODE(Assign, str_lit("Variable Assignment"), struct {\
AstNode* value;\
})\
AST_NODE(VarDecl, str_lit("Variable Declaration"), struct {\
P_Type* type;\
AstNode* value;\
})\
AST_NODE(STMT_END, str_lit(""), i8)

typedef u32 P_NodeType;
enum {
    NodeType_Error,
    
    NodeType_EXPR_START,
    NodeType_Ident,
    NodeType_IntLit,
    NodeType_BoolLit,
    NodeType_GlobalString,
    NodeType_Unary,
    NodeType_Binary,
    NodeType_Group,
    NodeType_Lambda,
    NodeType_Call,
    NodeType_EXPR_END,
    
    NodeType_STMT_START,
    NodeType_Return,
    NodeType_ExprStatement,
    NodeType_Block,
    NodeType_Assign,
    NodeType_VarDecl,
    NodeType_STMT_END,
};

// Uses TYPE(Id, Name)
#define BASIC_TYPES \
TYPE(Invalid, str_lit("Invalid")) \
TYPE(Integer, str_lit("Integer")) \
TYPE(Boolean, str_lit("Boolean")) \
TYPE(Void, str_lit("Void")) \
TYPE(Function, str_lit("Function")) \
TYPE(Cstring, str_lit("Cstring")) \
TYPE(Count, str_lit("__Count"))

typedef u32 P_BasicType;
enum {
    BasicType_Invalid,
    BasicType_Integer,
    BasicType_Boolean,
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
        struct { P_Type* return_type; P_Type** param_types; u32 arity; b8 varargs; } function;
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
    L_Token id;
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
    
    string filename;
    b8 errored;
    u8 error_count;
} P_Parser;

void P_Init(P_Parser* parser, string filename, L_Lexer* lexer);
AstNode* P_Parse(P_Parser* parser);
void P_Free(P_Parser* parser);

#endif //PARSER_H
