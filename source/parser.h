/* date = October 8th 2021 2:09 pm */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "defines.h"
#include "str.h"

#define TABLE_MAX_LOAD 0.75
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

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
    Prec_Unary,       // ! -
    Prec_Call,        // . ()
    Prec_Primary
};

typedef u32 P_ValueTypeCollection;
enum {
    ValueTypeCollection_Number,
    ValueTypeCollection_WholeNumber,
    ValueTypeCollection_DecimalNumber,
    ValueTypeCollection_String,
    ValueTypeCollection_Char,
    ValueTypeCollection_Bool,
};

typedef string P_ValueType;

typedef u32 P_ExprType;
enum {
    ExprType_IntLit, ExprType_LongLit, ExprType_FloatLit, ExprType_DoubleLit,
    ExprType_StringLit, ExprType_CharLit, ExprType_BoolLit,
    ExprType_Unary, ExprType_Binary, ExprType_Assignment, ExprType_Variable,
    ExprType_FuncCall, ExprType_Dot
};

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
        struct { L_TokenType operator; P_Expr* left; P_Expr* right; } binary;
        struct { L_TokenType operator; P_Expr* operand; } unary;
        struct { P_Expr* left; string right; } dot;
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
        struct { string name; u32 arity; string_list param_types; string_list param_names; P_ValueType type; P_Stmt* block; } func_decl;
        struct { string name; u32 member_count; string_list member_types; string_list member_names; } struct_decl;
        
        struct { P_Expr* condition; P_Stmt* then; } if_s;
        struct { P_Expr* condition; P_Stmt* then; P_Stmt* else_s; } if_else;
        struct { P_Expr* condition; P_Stmt* then; } while_s;
        struct { P_Expr* condition; P_Stmt* then; } do_while;
    } op;
};

typedef struct var_entry_key {
    string name;
    u32 depth;
} var_entry_key;

typedef struct var_table_entry {
    var_entry_key key;
    P_ValueType value;
} var_table_entry;

typedef struct var_hash_table {
    u32 count;
    u32 capacity;
    var_table_entry* entries;
} var_hash_table;

void var_hash_table_init(var_hash_table* table);
void var_hash_table_free(var_hash_table* table);
b8   var_hash_table_get(var_hash_table* table, var_entry_key key, P_ValueType* value);
b8   var_hash_table_set(var_hash_table* table, var_entry_key key, P_ValueType  value);
b8   var_hash_table_del(var_hash_table* table, var_entry_key key);
void var_hash_table_add_all(var_hash_table* from, var_hash_table* to);


typedef struct func_entry_key {
    string name;
    u32 depth;
} func_entry_key;

typedef struct func_table_entry {
    func_entry_key key;
    P_ValueType value;
} func_table_entry;

typedef struct func_hash_table {
    u32 count;
    u32 capacity;
    func_table_entry* entries;
} func_hash_table;

void func_hash_table_init(func_hash_table* table);
void func_hash_table_free(func_hash_table* table);
b8   func_hash_table_get(func_hash_table* table, func_entry_key key, P_ValueType* value);
b8   func_hash_table_set(func_hash_table* table, func_entry_key key, P_ValueType  value);
b8   func_hash_table_del(func_hash_table* table, func_entry_key key);
void func_hash_table_add_all(func_hash_table* from, func_hash_table* to);

typedef struct P_Struct {
    string name;
    u32 depth;
    u32 member_count;
    string_list member_types;
    string_list member_names;
} P_Struct;

typedef struct struct_array {
    u32 capacity;
    u32 count;
    P_Struct* elements;
} struct_array;

void struct_array_init(struct_array* array);
void struct_array_free(struct_array* array);
void struct_array_add(struct_array* array, P_Struct structure);

typedef struct P_Parser {
    M_Arena arena;
    L_Lexer lexer;
    
    P_Stmt* root;
    
    L_Token current;
    L_Token previous;
    
    u32 scope_depth;
    var_hash_table variables;
    func_hash_table functions;
    struct_array structures;
    
    b8 had_error;
    b8 panik_mode;
    
    b8 is_directly_in_func_body;
    b8 all_code_paths_return;
    P_ValueType function_body_ret;
    b8 encountered_return;
} P_Parser;

void P_Advance(P_Parser* parser);
void P_Initialize(P_Parser* parser, string source);
void P_Parse(P_Parser* parser);
void P_Free(P_Parser* parser);
void P_PrintExprAST(P_Expr* expr);
void P_PrintAST(P_Stmt* stmt);

#endif //PARSER_H
