/* date = October 8th 2021 2:09 pm */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "defines.h"
#include "str.h"
#include "types.h"
#include "data_structures.h"

typedef u32 P_Precedence;
enum {
    Prec_None,
    Prec_Assignment,  // =
    Prec_LogOr,       // ||
    Prec_LogAnd,      // &&
    Prec_BitOr,       // |
    Prec_BitXor,      // ^
    Prec_BitAnd,      // &
    Prec_Equality,    // == !=
    Prec_Comparison,  // < > <= >=
    Prec_Term,        // + -
    Prec_Factor,      // * /
    Prec_Unary,       // ! - *(deref) &(addr) ~
    Prec_Call,        // . () []
    Prec_Primary
};

typedef u32 P_ValueTypeCollection;
enum {
    ValueTypeCollection_Number,
    ValueTypeCollection_Pointer,
    ValueTypeCollection_WholeNumber,
    ValueTypeCollection_DecimalNumber,
    ValueTypeCollection_String,
    ValueTypeCollection_Char,
    ValueTypeCollection_Bool,
};

typedef u32 P_ExprType;
enum {
    ExprType_IntLit, ExprType_LongLit, ExprType_FloatLit, ExprType_DoubleLit,
    ExprType_StringLit, ExprType_CharLit, ExprType_BoolLit, ExprType_Typename,
    ExprType_Funcname, ExprType_Unary, ExprType_Binary, ExprType_Assignment,
    ExprType_Variable, ExprType_FuncCall, ExprType_Dot, ExprType_EnumDot,
    ExprType_Cast, ExprType_Index, ExprType_Addr, ExprType_Deref,
    ExprType_Nullptr, ExprType_ArrayLit, ExprType_Lambda,
};

struct P_Stmt;

typedef struct P_Expr P_Expr;
struct P_Expr {
    P_ExprType type;
    P_ValueType ret_type;
    b8 can_assign;
    union {
        i32    integer_lit;
        i64    long_lit;
        f32    float_lit;
        f64    double_lit;
        b8     bool_lit;
        string char_lit; // Is a string for transpiling reasons. :(
        string string_lit;
        P_ValueType typename;
        string funcname;
        string lambda;
        P_Expr* cast;
        P_Expr* addr;
        P_Expr* deref;
        expr_array array;
        struct { L_TokenType operator; P_Expr* left; P_Expr* right; } binary;
        struct { L_TokenType operator; P_Expr* operand; } unary;
        struct { P_Expr* left; string right; } dot;
        struct { P_Expr* operand; P_Expr* index; } index;
        struct { string left; string right; } enum_dot;
        struct { P_Expr* name; P_Expr* value; } assignment;
        string variable;
        struct { string name; P_Expr** params; u32 call_arity; } func_call;
    } op;
};

typedef u32 P_StmtType;
enum {
    StmtType_Expression, StmtType_Block, StmtType_Return, StmtType_If,
    StmtType_IfElse, StmtType_While, StmtType_DoWhile, StmtType_VarDecl,
    StmtType_VarDeclAssign, StmtType_FuncDecl, StmtType_NativeFuncDecl, StmtType_StructDecl,
    StmtType_EnumDecl, StmtType_For,
};

typedef struct P_Stmt P_Stmt;
struct P_Stmt {
    P_StmtType type;
    P_Stmt* next;
    union {
        P_Expr* expression;
        P_Expr* returned;
        P_Stmt* block;
        string native_func_decl;
        
        struct { P_ValueType type; string name; } var_decl;
        struct { P_ValueType type; string name; P_Expr* val; } var_decl_assign;
        struct { string name; u32 arity; value_type_list param_types; string_list param_names; P_ValueType type; P_Stmt* block; b8 varargs; } func_decl;
        struct { string name; u32 member_count; value_type_list member_types; string_list member_names; } struct_decl;
        struct { string name; u32 member_count; string_list member_names; } enum_decl;
        
        struct { P_Expr* condition; P_Stmt* then; } if_s;
        struct { P_Expr* condition; P_Stmt* then; P_Stmt* else_s; } if_else;
        struct { P_Expr* condition; P_Stmt* then; } while_s;
        struct { P_Expr* condition; P_Stmt* then; } do_while;
        struct { P_Expr* condition; P_Stmt* then; P_Stmt* init; P_Expr* loopexec; } for_s;
    } op;
};

typedef u32 P_PreStmtType;
enum {
    PreStmtType_ForwardDecl
};

typedef struct P_PreStmt P_PreStmt;
struct P_PreStmt {
    P_PreStmtType type;
    P_PreStmt* next;
    union {
        struct { string name; u32 arity; value_type_list param_types; string_list param_names; P_ValueType type; } forward_decl;
    } op;
};

#include "data_structures.h"

typedef struct P_Parser {
    M_Arena arena;
    L_Lexer lexer;
    string source;
    
    P_PreStmt* pre_root;
    P_Stmt* root;
    
    P_Stmt* lambda_functions_start;
    P_Stmt* lambda_functions_curr;
    
    L_Token next_two;
    L_Token next;
    L_Token current;
    L_Token previous;
    L_Token previous_two;
    
    u32 scope_depth;
    var_hash_table variables;
    func_hash_table functions;
    type_array types;
    
    P_ValueType* expected_fnptr;
    
    b8 had_error;
    b8 panik_mode;
    
    b8 is_directly_in_func_body;
    b8 all_code_paths_return;
    P_ValueType function_body_ret;
    b8 encountered_return;
    u32 lambda_number;
} P_Parser;

void P_Advance(P_Parser* parser);
void P_Initialize(P_Parser* parser, string source);
void P_PreParse(P_Parser* parser);
void P_Parse(P_Parser* parser);
void P_Free(P_Parser* parser);
void P_PrintExprAST(P_Expr* expr);
void P_PrintAST(P_Stmt* stmt);

#endif //PARSER_H
