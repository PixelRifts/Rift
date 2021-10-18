#include "parser.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "operator_bindings.h"


static const string ValueType_Invalid = str_lit("INVALID");

static const string ValueType_Integer = str_lit("int");
static const string ValueType_Long = str_lit("long");
static const string ValueType_Float = str_lit("float");
static const string ValueType_Double = str_lit("double");

static const string ValueType_String = str_lit("string");
static const string ValueType_Char = str_lit("char");
static const string ValueType_Bool = str_lit("bool");

static const string ValueType_MaxCount = str_lit("max_ct");


//~ Variable Hashtable
static u32 hash_var_key(var_entry_key k) {
    u32 hash = 2166136261u;
    for (u32 i = 0; i < k.name.size; i++) {
        hash ^= k.name.str[i];
        hash *= 16777619;
    }
    hash += k.depth;
    return hash;
}

static var_table_entry* find_var_entry(var_table_entry* entries, i32 cap, var_entry_key key) {
    u32 idx = hash_var_key(key) % cap;
    var_table_entry* tombstone_e = nullptr;
    
    while (true) {
        var_table_entry* entry = &entries[idx];
        if (entry->key.name.size == 0) {
            if (entry->value.size == 0)
                return tombstone_e != nullptr ? tombstone_e : entry;
            else
                if (tombstone_e == nullptr) tombstone_e = entry;
        } else if (entry->key.name.size == key.name.size)
            if (memcmp(entry->key.name.str, key.name.str, key.name.size) == 0 && entry->key.depth == key.depth) {
            return entry;
        }
        idx = (idx + 1) % cap;
    }
}

static void var_table_adjust_cap(var_hash_table* table, i32 cap) {
    var_table_entry* entries = malloc(sizeof(var_table_entry) * cap);
    memset(entries, 0, sizeof(var_table_entry) * cap);
    
    table->count = 0;
    for (u32 i = 0; i < table->capacity; i++) {
        var_table_entry* entry = &table->entries[i];
        if (entry->key.name.size == 0) continue;
        
        var_table_entry* dest = find_var_entry(entries, cap, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }
    
    free(table->entries);
    table->entries = entries;
    table->capacity = cap;
}

void var_hash_table_init(var_hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = nullptr;
}

void var_hash_table_free(var_hash_table* table) {
    free(table->entries);
    table->count = 0;
    table->capacity = 0;
    table->entries = nullptr;
}

b8 var_hash_table_get(var_hash_table* table, var_entry_key key, P_ValueType* value) {
    if (table->count == 0) return false;
    var_table_entry* entry = find_var_entry(table->entries, table->capacity, key);
    if (entry->key.name.size == 0) return false;
    *value = entry->value;
    return true;
}

b8 var_hash_table_set(var_hash_table* table, var_entry_key key, P_ValueType value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        i32 cap = GROW_CAPACITY(table->capacity);
        var_table_adjust_cap(table, cap);
    }
    
    var_table_entry* entry = find_var_entry(table->entries, table->capacity, key);
    b8 is_new_key = entry->key.name.size == 0;
    if (is_new_key && entry->value.size == 0)
        table->count++;
    
    entry->key = key;
    entry->value = value;
    return is_new_key;
}

b8 var_hash_table_del(var_hash_table* table, var_entry_key key) {
    if (table->count == 0) return false;
    var_table_entry* entry = find_var_entry(table->entries, table->capacity, key);
    if (entry->key.name.size == 0) return false;
    entry->key = (var_entry_key) {0};
    entry->value = ValueType_MaxCount;
    return true;
}

void var_hash_table_add_all(var_hash_table* from, var_hash_table* to) {
    for (u32 i = 0; i < from->capacity; i++) {
        var_table_entry* entry = &from->entries[i];
        if (entry->key.name.size != 0) {
            var_hash_table_set(to, entry->key, entry->value);
        }
    }
}

//~ Function hashtable
static u32 hash_func_key(func_entry_key k) {
    u32 hash = 2166136261u;
    for (u32 i = 0; i < k.name.size; i++) {
        hash ^= k.name.str[i];
        hash *= 16777619;
    }
    hash += k.depth;
    return hash;
}

static func_table_entry* find_func_entry(func_table_entry* entries, i32 cap, func_entry_key key) {
    u32 idx = hash_func_key(key) % cap;
    func_table_entry* tombstone_e = nullptr;
    
    while (true) {
        func_table_entry* entry = &entries[idx];
        if (entry->key.name.size == 0) {
            if (entry->value.size == 0)
                return tombstone_e != nullptr ? tombstone_e : entry;
            else
                if (tombstone_e == nullptr) tombstone_e = entry;
        } else if (str_eq(entry->key.name, key.name) && entry->key.depth == key.depth)
            return entry;
        idx = (idx + 1) % cap;
    }
}

static void func_table_adjust_cap(func_hash_table* table, i32 cap) {
    func_table_entry* entries = malloc(sizeof(func_table_entry) * cap);
    memset(entries, 0, sizeof(func_table_entry) * cap);
    
    table->count = 0;
    for (u32 i = 0; i < table->capacity; i++) {
        func_table_entry* entry = &table->entries[i];
        if (entry->key.name.size == 0) continue;
        
        func_table_entry* dest = find_func_entry(entries, cap, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }
    
    free(table->entries);
    table->entries = entries;
    table->capacity = cap;
}

void func_hash_table_init(func_hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = nullptr;
}

void func_hash_table_free(func_hash_table* table) {
    free(table->entries);
    table->count = 0;
    table->capacity = 0;
    table->entries = nullptr;
}

b8 func_hash_table_get(func_hash_table* table, func_entry_key key, P_ValueType* value) {
    if (table->count == 0) return false;
    func_table_entry* entry = find_func_entry(table->entries, table->capacity, key);
    if (entry->key.name.size == 0) return false;
    *value = entry->value;
    return true;
}

b8 func_hash_table_set(func_hash_table* table, func_entry_key key, P_ValueType  value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        i32 cap = GROW_CAPACITY(table->capacity);
        func_table_adjust_cap(table, cap);
    }
    
    func_table_entry* entry = find_func_entry(table->entries, table->capacity, key);
    b8 is_new_key = entry->key.name.size == 0;
    if (is_new_key && entry->value.size == 0)
        table->count++;
    
    entry->key = key;
    entry->value = value;
    return is_new_key;
}

b8 func_hash_table_del(func_hash_table* table, func_entry_key key) {
    if (table->count == 0) return false;
    func_table_entry* entry = find_func_entry(table->entries, table->capacity, key);
    if (entry->key.name.size == 0) return false;
    entry->key = (func_entry_key) {0};
    entry->value = ValueType_MaxCount;
    return true;
}


void func_hash_table_add_all(func_hash_table* from, func_hash_table* to) {
    for (u32 i = 0; i < from->capacity; i++) {
        func_table_entry* entry = &from->entries[i];
        if (entry->key.name.size != 0) {
            func_hash_table_set(to, entry->key, entry->value);
        }
    }
}

//~ Struct List
void struct_array_init(struct_array* array) {
    array->elements = calloc(8, sizeof(struct_array));
    array->count = 0;
    array->capacity = 8;
}

void struct_array_free(struct_array* array) {
    free(array->elements);
    array->count = 0;
    array->capacity = 0;
}

void struct_array_add(struct_array* array, P_Struct structure) {
    if (array->count + 1 > array->capacity) {
        void* prev = array->elements;
        array->elements = malloc(array->capacity * 2);
        memmove(array->elements, prev, array->count * sizeof(P_Struct));
        free(prev);
    }
    *(array->elements + array->count) = structure;
    array->count++;
}

static P_Struct* struct_array_get(P_Parser* parser, string struct_name, u32 depth) {
    for (u32 i = 0; i < parser->structures.count; i++) {
        if (str_eq(struct_name, parser->structures.elements[i].name) && parser->structures.elements[i].depth <= depth)
            return &parser->structures.elements[i];
    }
    return nullptr;
}

static b8 structure_exists(P_Parser* parser, string struct_name, u32 depth) {
    for (u32 i = 0; i < parser->structures.count; i++) {
        if (str_eq(struct_name, parser->structures.elements[i].name) && parser->structures.elements[i].depth <= depth)
            return true;
    }
    return false;
}

static P_ValueType member_type_get(P_Struct* structure, string reqd) {
    for (u32 i = 0; i < structure->member_count; i++) {
        if (str_eq(structure->member_names[i], reqd))
            return structure->member_types[i];
    }
    return ValueType_Invalid;
}

static b8 member_exists(P_Struct* structure, string reqd) {
    for (u32 i = 0; i < structure->member_count; i++) {
        if (str_eq(structure->member_names[i], reqd))
            return true;
    }
    return false;
}

//~ Errors
static void report_error_at(P_Parser* parser, L_Token* token, string message, ...) {
    if (parser->panik_mode) exit(1);
    
    fprintf(stderr, "Error:%d: ", token->line);
    if (token->type != TokenType_Error && token->type != TokenType_EOF)
        fprintf(stderr, "At '%.*s' ", token->length, token->start);
    
    va_list args;
    va_start(args, message);
    vfprintf(stderr, (char*)message.str, args);
    va_end(args);
    
    parser->had_error = true;
    parser->panik_mode = true;
}

#define report_error(parser, message, ...) report_error_at(parser, &parser->previous, message, ##__VA_ARGS__)
#define report_error_at_current(parser, message, ...) report_error_at(parser, &parser->current, message, ##__VA_ARGS__)

//~ Elpers
void P_Advance(P_Parser* parser) {
    parser->previous = parser->current;
    
    while (true) {
        parser->current = L_LexToken(&parser->lexer);
        if (parser->current.type != TokenType_Error) break;
        report_error_at_current(parser, (string) { .str = (u8*)parser->current.start, .size = parser->current.length });
    }
}

static void P_Consume(P_Parser* parser, L_TokenType type, string message) {
    if (parser->current.type == type) { P_Advance(parser); return; }
    report_error_at_current(parser, message);
}

static void P_ConsumeType(P_Parser* parser, string message) {
    if (parser->current.type == TokenType_Int || parser->current.type == TokenType_Long ||
        parser->current.type == TokenType_Float || parser->current.type == TokenType_Double || parser->current.type == TokenType_Bool || parser->current.type == TokenType_Char) { P_Advance(parser); return; }
    
    report_error_at_current(parser, message);
}

static b8 P_Match(P_Parser* parser, L_TokenType expected) {
    if (parser->current.type != expected)
        return false;
    P_Advance(parser);
    return true;
}

static void P_Sync(P_Parser* parser) {
    if (parser->panik_mode) {
        parser->panik_mode = false;
        while (parser->current.type != TokenType_EOF) {
            if (parser->previous.type == TokenType_Semicolon) return;
            else P_Advance(parser);
        }
    }
}

static b8 P_IsTypeToken(P_Parser* parser) {
    if (P_Match(parser, TokenType_Int)    ||
        P_Match(parser, TokenType_Long)   ||
        P_Match(parser, TokenType_Float)  ||
        P_Match(parser, TokenType_Double) ||
        P_Match(parser, TokenType_Char)   ||
        P_Match(parser, TokenType_Bool)) return true;
    if (parser->current.type == TokenType_Identifier) {
        if (structure_exists(parser, (string) { .str = parser->current.start, .size = parser->current.length }, parser->scope_depth)) {
            P_Advance(parser);
            return true;
        }
    }
    return false;
}

static P_ValueType P_TypeTokenToValueType(P_Parser* parser) {
    switch (parser->previous.type) {
        case TokenType_Int:    return ValueType_Integer;
        case TokenType_Long:   return ValueType_Long;
        case TokenType_Float:  return ValueType_Float;
        case TokenType_Double: return ValueType_Double;
        case TokenType_Char:   return ValueType_Char;
        case TokenType_Bool:   return ValueType_Bool;
    }
    return (P_ValueType) { .str = parser->previous.start, .size = parser->previous.length };
}

static string P_FuncNameMangle(P_Parser* parser, string name, u32 arity, P_ValueType* params, string additional_info) {
    string_list sl = {0};
    string_list_push(&parser->arena, &sl, str_from_format(&parser->arena, "%.*s_%u", name.size, name.str, arity));
    for (u32 i = 0; i < arity; i++)
        string_list_push(&parser->arena, &sl, str_from_format(&parser->arena, "%s", params[i].str));
    if (additional_info.size != 0) string_list_push(&parser->arena, &sl, str_from_format(&parser->arena, "_%s", additional_info));
    return string_list_flatten(&parser->arena, &sl);
}

//~ Binding
static b8 P_CheckValueType(P_ValueTypeCollection expected, P_ValueType type) {
    if (expected == ValueTypeCollection_Number) {
        if (str_eq(type, ValueType_Integer) || str_eq(type, ValueType_Long) || str_eq(type, ValueType_Float) || str_eq(type, ValueType_Double))
            return true;
    } else if (expected == ValueTypeCollection_WholeNumber) {
        if (str_eq(type, ValueType_Integer) || str_eq(type, ValueType_Long)) return true;
    } else if (expected == ValueTypeCollection_DecimalNumber) {
        if (str_eq(type, ValueType_Double) || str_eq(type, ValueType_Float)) return true;
    } else if (expected == ValueTypeCollection_String) {
        if (str_eq(type, ValueType_String)) return true;
    } else if (expected == ValueTypeCollection_Char) {
        if (str_eq(type, ValueType_Char)) return true;
    } else if (expected == ValueTypeCollection_Bool) {
        if (str_eq(type, ValueType_Bool)) return true;
    }
    return false;
}

// NOTE(voxel): Wack name but it's what I know : Basically a typecheck
static void P_Bind(P_Parser* parser, P_Expr* expr, P_ValueTypeCollection* expected, u32 flagct, string error_msg) {
    for (u32 i = 0; i < flagct; i++) {
        if (P_CheckValueType(expected[i], expr->ret_type)) {
            return;
        }
    }
    report_error(parser, error_msg, expr->ret_type.str);
}

static void P_BindDouble(P_Parser* parser, P_Expr* left, P_Expr* right, P_BinaryOpPair* expected, u32 flagct, string error_msg) {
    for (u32 i = 0; i < flagct; i++) {
        if (P_CheckValueType(expected[i].left, left->ret_type) && P_CheckValueType(expected[i].right, right->ret_type)) {
            return;
        }
    }
    report_error(parser, error_msg, left->ret_type.str, right->ret_type.str);
}

// Basic Max function
static P_ValueType P_GetNumberBinaryValType(P_Expr* a, P_Expr* b) {
    if (str_eq(a->ret_type, ValueType_Double) || str_eq(b->ret_type, ValueType_Double))
        return ValueType_Double;
    if (str_eq(a->ret_type, ValueType_Float) || str_eq(b->ret_type, ValueType_Float))
        return ValueType_Float;
    if (str_eq(a->ret_type, ValueType_Long) || str_eq(b->ret_type, ValueType_Long))
        return ValueType_Long;
    if (str_eq(a->ret_type, ValueType_Integer) || str_eq(b->ret_type, ValueType_Integer))
        return ValueType_Integer;
    return ValueType_Invalid;
}

//~ Parse Rules
#if 0
typedef syntax_highlight_only_lmao P_PrefixParseFn;
typedef syntax_highlight_only_too_lmao P_InfixParseFn;
#endif

typedef P_Expr* (*P_PrefixParseFn)(P_Parser*);
typedef P_Expr* (*P_InfixParseFn)(P_Parser*, P_Expr*);

typedef struct P_ParseRule {
    P_PrefixParseFn prefix;
    P_InfixParseFn infix;
    P_Precedence precedence;
} P_ParseRule;

//~ AST Nodes
static P_Expr* P_MakeIntNode(P_Parser* parser, i32 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_IntLit;
    expr->ret_type = ValueType_Integer;
    expr->can_assign = false;
    expr->op.integer_lit = value;
    return expr;
}

static P_Expr* P_MakeLongNode(P_Parser* parser, i64 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_LongLit;
    expr->ret_type = ValueType_Long;
    expr->can_assign = false;
    expr->op.long_lit = value;
    return expr;
}

static P_Expr* P_MakeFloatNode(P_Parser* parser, f32 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_FloatLit;
    expr->ret_type = ValueType_Long;
    expr->can_assign = false;
    expr->op.float_lit = value;
    return expr;
}

static P_Expr* P_MakeDoubleNode(P_Parser* parser, f64 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_DoubleLit;
    expr->ret_type = ValueType_Double;
    expr->can_assign = false;
    expr->op.double_lit = value;
    return expr;
}

static P_Expr* P_MakeStringNode(P_Parser* parser, string value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_StringLit;
    expr->ret_type = ValueType_String;
    expr->can_assign = false;
    expr->op.string_lit = value;
    return expr;
}

static P_Expr* P_MakeCharNode(P_Parser* parser, string value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_CharLit;
    expr->ret_type = ValueType_Char;
    expr->can_assign = false;
    expr->op.char_lit = value;
    return expr;
}

static P_Expr* P_MakeBoolNode(P_Parser* parser, b8 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_BoolLit;
    expr->ret_type = ValueType_Bool;
    expr->can_assign = false;
    expr->op.bool_lit = value;
    return expr;
}

static P_Expr* P_MakeUnaryNode(P_Parser* parser, L_TokenType type, P_Expr* operand, P_ValueType ret_type) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Unary;
    expr->ret_type = ret_type;
    expr->can_assign = false;
    expr->op.unary.operator = type;
    expr->op.unary.operand = operand;
    return expr;
}

static P_Expr* P_MakeBinaryNode(P_Parser* parser, L_TokenType type, P_Expr* left, P_Expr* right, P_ValueType ret_type) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Binary;
    expr->ret_type = ret_type;
    expr->can_assign = false;
    expr->op.binary.operator = type;
    expr->op.binary.left = left;
    expr->op.binary.right = right;
    return expr;
}

static P_Expr* P_MakeAssignmentNode(P_Parser* parser, P_Expr* name, P_Expr* value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Assignment;
    expr->ret_type = value->ret_type;
    expr->can_assign = false;
    expr->op.assignment.name = name;
    expr->op.assignment.value = value;
    return expr;
}

static P_Expr* P_MakeDotNode(P_Parser* parser, P_ValueType type, P_Expr* left, string right) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Dot;
    expr->ret_type = type;
    expr->can_assign = true;
    expr->op.dot.left = left;
    expr->op.dot.right = right;
    return expr;
}

static P_Expr* P_MakeVariableNode(P_Parser* parser, string name, P_ValueType type) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Variable;
    expr->ret_type = type;
    expr->can_assign = true;
    expr->op.variable = name;
    return expr;
}

static P_Expr* P_MakeFuncCallNode(P_Parser* parser, string name, P_ValueType type, P_Expr** exprs, u32 call_arity) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_FuncCall;
    expr->ret_type = type;
    expr->can_assign = false;
    expr->op.func_call.name = name;
    expr->op.func_call.params = exprs;
    expr->op.func_call.call_arity = call_arity;
    return expr;
}

static P_Stmt* P_MakeBlockStmtNode(P_Parser* parser) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Block;
    stmt->next = nullptr;
    stmt->op.block = nullptr;
    return stmt;
}

static P_Stmt* P_MakeExpressionStmtNode(P_Parser* parser, P_Expr* expr) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Expression;
    stmt->next = nullptr;
    stmt->op.expression = expr;
    return stmt;
}

static P_Stmt* P_MakeReturnStmtNode(P_Parser* parser, P_Expr* expr) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Return;
    stmt->next = nullptr;
    stmt->op.returned = expr;
    return stmt;
}

static P_Stmt* P_MakeIfStmtNode(P_Parser* parser, P_Expr* condition, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_If;
    stmt->next = nullptr;
    stmt->op.if_s.condition = condition;
    stmt->op.if_s.then = then;
    return stmt;
}

static P_Stmt* P_MakeWhileStmtNode(P_Parser* parser, P_Expr* condition, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_While;
    stmt->next = nullptr;
    stmt->op.while_s.condition = condition;
    stmt->op.while_s.then = then;
    return stmt;
}

static P_Stmt* P_MakeDoWhileStmtNode(P_Parser* parser, P_Expr* condition, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_DoWhile;
    stmt->next = nullptr;
    stmt->op.do_while.condition = condition;
    stmt->op.do_while.then = then;
    return stmt;
}

static P_Stmt* P_MakeIfElseStmtNode(P_Parser* parser, P_Expr* condition, P_Stmt* then, P_Stmt* else_s) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_IfElse;
    stmt->next = nullptr;
    stmt->op.if_else.condition = condition;
    stmt->op.if_else.then = then;
    stmt->op.if_else.else_s = else_s;
    return stmt;
}

static P_Stmt* P_MakeVarDeclStmtNode(P_Parser* parser, P_ValueType type, string name) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_VarDecl;
    stmt->next = nullptr;
    stmt->op.var_decl.type = type;
    stmt->op.var_decl.name = name;
    return stmt;
}

static P_Stmt* P_MakeStructDeclStmtNode(P_Parser* parser, string name, u32 count, P_ValueType* types, string* names) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_StructDecl;
    stmt->next = nullptr;
    stmt->op.struct_decl.name = name;
    stmt->op.struct_decl.member_count = count;
    stmt->op.struct_decl.member_types = types;
    stmt->op.struct_decl.member_names = names;
    return stmt;
}

static P_Stmt* P_MakeFuncStmtNode(P_Parser* parser, P_ValueType type, string name, u32 arity, P_ValueType* param_types, string* param_names) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_FuncDecl;
    stmt->next = nullptr;
    stmt->op.func_decl.type = type;
    stmt->op.func_decl.name = name;
    stmt->op.func_decl.arity = arity;
    stmt->op.func_decl.param_types = param_types;
    stmt->op.func_decl.param_names = param_names;
    stmt->op.func_decl.block = nullptr;
    return stmt;
}

//~ Expressions
static P_Expr* P_ExprPrecedence(P_Parser* parser, P_Precedence precedence);
static P_ParseRule* P_GetRule(L_TokenType type);
static P_Expr* P_Expression(P_Parser* parser);

static P_Expr* P_ExprInteger(P_Parser* parser) {
    i32 value = atoi(parser->previous.start);
    return P_MakeIntNode(parser, value);
}

static P_Expr* P_ExprLong(P_Parser* parser) {
    i64 value = atoll(parser->previous.start);
    return P_MakeLongNode(parser, value);
}

static P_Expr* P_ExprFloat(P_Parser* parser) {
    f32 value = atof(parser->previous.start);
    return P_MakeFloatNode(parser, value);
}

static P_Expr* P_ExprDouble(P_Parser* parser) {
    f64 value = strtod(parser->previous.start, nullptr);
    return P_MakeDoubleNode(parser, value);
}

static P_Expr* P_ExprString(P_Parser* parser) {
    string s = { .str = (u8*)parser->previous.start, .size = parser->current.start - parser->previous.start };
    return P_MakeStringNode(parser, s);
}

static P_Expr* P_ExprChar(P_Parser* parser) {
    string c = { .str = (u8*)parser->previous.start, .size = parser->current.start - parser->previous.start };
    return P_MakeCharNode(parser, c);
}

static P_Expr* P_ExprBool(P_Parser* parser) {
    b8 b = parser->previous.type == TokenType_True;
    return P_MakeBoolNode(parser, b);
}

static P_Expr* P_ExprVar(P_Parser* parser) {
    string name = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
    if (P_Match(parser, TokenType_OpenParenthesis)) {
        P_Expr* params[256];
        P_ValueType param_types[256];
        u32 call_arity = 0;
        while (!P_Match(parser, TokenType_CloseParenthesis)) {
            if (call_arity != 0) {
                if (!P_Match(parser, TokenType_Comma)) {
                    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) or , after parameter\n"));
                    break;
                }
            }
            
            params[call_arity] = P_Expression(parser);
            param_types[call_arity] = params[call_arity]->ret_type;
            call_arity++;
        }
        
        string actual_name = P_FuncNameMangle(parser, name, call_arity, param_types, (string) {0});
        
        P_Expr** param_buffer = arena_alloc(&parser->arena, call_arity * sizeof(P_Expr*));
        memcpy(param_buffer, params, call_arity * sizeof(P_Expr*));
        
        func_entry_key key = { .name = actual_name, .depth = parser->scope_depth };
        P_ValueType value = ValueType_Invalid;
        while (key.depth != -1) {
            if (func_hash_table_get(&parser->functions, key, &value)) {
                return P_MakeFuncCallNode(parser, actual_name, value, param_buffer, call_arity);
            }
            key.depth--;
        }
        report_error(parser, str_lit("Undefined function %.*s with provided parameters\n"), name.size, name.str);
        
    } else {
        
        var_entry_key key = { .name = name, .depth = parser->scope_depth };
        P_ValueType value = ValueType_Invalid;
        while (key.depth != -1) {
            if (var_hash_table_get(&parser->variables, key, &value)) {
                return P_MakeVariableNode(parser, name, value);
            }
            key.depth--;
        }
        report_error(parser, str_lit("Undefined variable %.*s\n"), name.size, name.str);
        
    }
    return nullptr;
}

static P_Expr* P_ExprAssign(P_Parser* parser, P_Expr* left) {
    if (left != nullptr) {
        if (!left->can_assign)
            report_error(parser, str_lit("Required variable name before = sign\n"));
        P_Expr* name = left;
        
        P_Expr* xpr = P_Expression(parser);
        if (str_eq(xpr->ret_type, left->ret_type))
            return P_MakeAssignmentNode(parser, name, xpr);
        
        report_error(parser, str_lit("Cannot assign %.*s to variable\n"), xpr->ret_type.size, xpr->ret_type.str);
    }
    return nullptr;
}

static P_Expr* P_ExprGroup(P_Parser* parser) {
    P_Expr* in = P_Expression(parser);
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression\n"));
    return in;
}

static P_Expr* P_ExprUnary(P_Parser* parser) {
    L_TokenType op_type = parser->previous.type;
    P_Expr* operand = P_ExprPrecedence(parser, Prec_Unary);
    P_ValueType ret_type;
    switch (op_type) {
        case TokenType_Plus: {
            P_Bind(parser, operand, list_operator_arithmetic, sizeof(list_operator_arithmetic), str_lit("Cannot apply unary operator + to %s\n"));
            ret_type = operand->ret_type;
        } break;
        case TokenType_Minus: {
            P_Bind(parser, operand, list_operator_arithmetic, sizeof(list_operator_arithmetic), str_lit("Cannot apply unary operator - to %s\n"));
            ret_type = operand->ret_type;
        } break;
        case TokenType_Tilde: {
            P_Bind(parser, operand, list_operator_bin, sizeof(list_operator_bin), str_lit("Cannot apply unary operator ~ to %s\n"));
            ret_type = operand->ret_type;
        } break;
        case TokenType_Bang: {
            P_Bind(parser, operand, list_operator_logical, sizeof(list_operator_logical), str_lit("Cannot apply unary operator ! to %s\n"));
            ret_type = operand->ret_type;
        } break;
    }
    return P_MakeUnaryNode(parser, op_type, operand, ret_type);
}

static P_Expr* P_ExprBinary(P_Parser* parser, P_Expr* left) {
    L_TokenType op_type = parser->previous.type;
    P_ParseRule* rule = P_GetRule(op_type);
    P_Expr* right = P_ExprPrecedence(parser, (P_Precedence)(rule->precedence + 1));
    P_ValueType ret_type;
    switch (op_type) {
        case TokenType_Plus: {
            P_BindDouble(parser, left, right, pairs_operator_arithmetic, sizeof(pairs_operator_arithmetic), str_lit("Cannot apply binary operator + to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Minus: {
            P_BindDouble(parser, left, right, pairs_operator_arithmetic, sizeof(pairs_operator_arithmetic), str_lit("Cannot apply binary operator - to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Star: {
            P_BindDouble(parser, left, right, pairs_operator_arithmetic, sizeof(pairs_operator_arithmetic), str_lit("Cannot apply binary operator * to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Slash: {
            P_BindDouble(parser, left, right, pairs_operator_arithmetic, sizeof(pairs_operator_arithmetic), str_lit("Cannot apply binary operator / to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Hat: {
            P_BindDouble(parser, left, right, pairs_operator_bin, sizeof(pairs_operator_bin), str_lit("Cannot apply binary operator ^ to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Ampersand: {
            P_BindDouble(parser, left, right, pairs_operator_bin, sizeof(pairs_operator_bin), str_lit("Cannot apply binary operator & to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Pipe: {
            P_BindDouble(parser, left, right, pairs_operator_bin, sizeof(pairs_operator_bin), str_lit("Cannot apply binary operator | to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_AmpersandAmpersand: {
            P_BindDouble(parser, left, right, pairs_operator_logical, sizeof(pairs_operator_logical), str_lit("Cannot apply binary operator && to %s and %s\n"));
            ret_type = ValueType_Bool;
        } break;
        
        case TokenType_PipePipe: {
            P_BindDouble(parser, left, right, pairs_operator_logical, sizeof(pairs_operator_logical), str_lit("Cannot apply binary operator || to %s and %s\n"));
            ret_type = ValueType_Bool;
        } break;
        
        case TokenType_EqualEqual: {
            P_BindDouble(parser, left, right, pairs_operator_equality, sizeof(pairs_operator_equality), str_lit("Cannot apply binary operator == to %s and %s\n"));
            ret_type = ValueType_Bool;
        } break;
        
        case TokenType_BangEqual: {
            P_BindDouble(parser, left, right, pairs_operator_equality, sizeof(pairs_operator_equality), str_lit("Cannot apply binary operator != to %s and %s\n"));
            ret_type = ValueType_Bool;
        } break;
        
        case TokenType_Less: {
            P_BindDouble(parser, left, right, pairs_operator_cmp, sizeof(pairs_operator_cmp), str_lit("Cannot apply binary operator < to %s and %s\n"));
            ret_type = ValueType_Bool;
        } break;
        
        case TokenType_Greater: {
            P_BindDouble(parser, left, right, pairs_operator_cmp, sizeof(pairs_operator_cmp), str_lit("Cannot apply binary operator > to %s and %s\n"));
            ret_type = ValueType_Bool;
        } break;
        
        case TokenType_LessEqual: {
            P_BindDouble(parser, left, right, pairs_operator_cmp, sizeof(pairs_operator_cmp), str_lit("Cannot apply binary operator <= to %s and %s\n"));
            ret_type = ValueType_Bool;
        } break;
        
        case TokenType_GreaterEqual: {
            P_BindDouble(parser, left, right, pairs_operator_cmp, sizeof(pairs_operator_cmp), str_lit("Cannot apply binary operator >= to %s and %s\n"));
            ret_type = ValueType_Bool;
        } break;
    }
    return P_MakeBinaryNode(parser, op_type, left, right, ret_type);
}

static P_Expr* P_ExprDot(P_Parser* parser, P_Expr* left) {
    P_ValueType type = left->ret_type;
    
    if (!structure_exists(parser, type, parser->scope_depth)) {
        report_error(parser, str_lit("Cannot apply . operator\n"));
    }
    
    P_Consume(parser, TokenType_Identifier, str_lit("Expected Member name after .\n"));
    string reqd = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
    P_Struct* structure = struct_array_get(parser, type, parser->scope_depth);
    if (!member_exists(structure, reqd)) {
        report_error(parser, str_lit("No member %.*s in struct %.*s\n"), reqd.size, reqd.str, type.size, type.str);
    }
    P_ValueType member_type = member_type_get(structure, reqd);
    
    return P_MakeDotNode(parser, member_type, left, reqd);
}

P_ParseRule parse_rules[] = {
    [TokenType_Error]            = { nullptr, nullptr, Prec_None },
    [TokenType_EOF]              = { nullptr, nullptr, Prec_None },
    [TokenType_Whitespace]       = { nullptr, nullptr, Prec_None },
    [TokenType_Identifier]       = { P_ExprVar    , nullptr, Prec_None },
    [TokenType_IntLit]           = { P_ExprInteger, nullptr, Prec_None },
    [TokenType_FloatLit]         = { P_ExprFloat  , nullptr, Prec_None },
    [TokenType_DoubleLit]        = { P_ExprDouble , nullptr, Prec_None },
    [TokenType_LongLit]          = { P_ExprLong   , nullptr, Prec_None },
    [TokenType_StringLit]        = { P_ExprString , nullptr, Prec_None },
    [TokenType_CharLit]          = { P_ExprChar   , nullptr, Prec_None },
    [TokenType_Plus]             = { P_ExprUnary, P_ExprBinary, Prec_Term },
    [TokenType_Minus]            = { P_ExprUnary, P_ExprBinary, Prec_Term },
    [TokenType_Star]             = { nullptr, P_ExprBinary, Prec_Factor },
    [TokenType_Slash]            = { nullptr, P_ExprBinary, Prec_Factor },
    [TokenType_PlusPlus]         = { nullptr, nullptr, Prec_None },
    [TokenType_MinusMinus]       = { nullptr, nullptr, Prec_None },
    [TokenType_Backslash]        = { nullptr, nullptr, Prec_None },
    [TokenType_Hat]              = { nullptr, P_ExprBinary, Prec_BitXor },
    [TokenType_Ampersand]        = { nullptr, P_ExprBinary, Prec_BitAnd },
    [TokenType_Pipe]             = { nullptr, P_ExprBinary, Prec_BitOr },
    [TokenType_Tilde]            = { P_ExprUnary, nullptr,  Prec_None },
    [TokenType_Bang]             = { P_ExprUnary, nullptr,  Prec_None },
    [TokenType_Equal]            = { nullptr, P_ExprAssign, Prec_Assignment },
    [TokenType_PlusEqual]        = { nullptr, nullptr, Prec_None },
    [TokenType_MinusEqual]       = { nullptr, nullptr, Prec_None },
    [TokenType_StarEqual]        = { nullptr, nullptr, Prec_None },
    [TokenType_SlashEqual]       = { nullptr, nullptr, Prec_None },
    [TokenType_AmpersandEqual]   = { nullptr, nullptr, Prec_None },
    [TokenType_PipeEqual]        = { nullptr, nullptr, Prec_None },
    [TokenType_HatEqual]         = { nullptr, nullptr, Prec_None },
    [TokenType_TildeEqual]       = { nullptr, nullptr, Prec_None },
    [TokenType_EqualEqual]       = { nullptr, P_ExprBinary, Prec_Equality },
    [TokenType_BangEqual]        = { nullptr, P_ExprBinary, Prec_Equality },
    [TokenType_Less]             = { nullptr, P_ExprBinary, Prec_Comparison },
    [TokenType_Greater]          = { nullptr, P_ExprBinary, Prec_Comparison },
    [TokenType_LessEqual]        = { nullptr, P_ExprBinary, Prec_Comparison },
    [TokenType_GreaterEqual]     = { nullptr, P_ExprBinary, Prec_Comparison },
    [TokenType_AmpersandAmpersand] = { nullptr, P_ExprBinary, Prec_LogAnd },
    [TokenType_PipePipe]         = { nullptr, P_ExprBinary, Prec_LogOr },
    [TokenType_OpenBrace]        = { nullptr, nullptr, Prec_None },
    [TokenType_OpenParenthesis]  = { P_ExprGroup, nullptr, Prec_None },
    [TokenType_OpenBracket]      = { nullptr, nullptr, Prec_None },
    [TokenType_CloseBrace]       = { nullptr, nullptr, Prec_None },
    [TokenType_CloseParenthesis] = { nullptr, nullptr, Prec_None },
    [TokenType_CloseBracket]     = { nullptr, nullptr, Prec_None },
    [TokenType_Dot]              = { nullptr, P_ExprDot, Prec_Call },
    [TokenType_Semicolon]        = { nullptr, nullptr, Prec_None },
    [TokenType_Colon]            = { nullptr, nullptr, Prec_None },
    [TokenType_Question]         = { nullptr, nullptr, Prec_None },
    [TokenType_Return]           = { nullptr, nullptr, Prec_None },
    [TokenType_Struct]           = { nullptr, nullptr, Prec_None },
    [TokenType_Enum]             = { nullptr, nullptr, Prec_None },
    [TokenType_Null]             = { nullptr, nullptr, Prec_None },
    [TokenType_Nullptr]          = { nullptr, nullptr, Prec_None },
    [TokenType_If]               = { nullptr, nullptr, Prec_None },
    [TokenType_Else]             = { nullptr, nullptr, Prec_None },
    [TokenType_Do]               = { nullptr, nullptr, Prec_None },
    [TokenType_For]              = { nullptr, nullptr, Prec_None },
    [TokenType_While]            = { nullptr, nullptr, Prec_None },
    [TokenType_True]             = { P_ExprBool, nullptr, Prec_None },
    [TokenType_False]            = { P_ExprBool, nullptr, Prec_None },
    [TokenType_Int]              = { nullptr, nullptr, Prec_None },
    [TokenType_Float]            = { nullptr, nullptr, Prec_None },
    [TokenType_Bool]             = { nullptr, nullptr, Prec_None },
    [TokenType_Double]           = { nullptr, nullptr, Prec_None },
    [TokenType_Char]             = { nullptr, nullptr, Prec_None },
    [TokenType_Long]             = { nullptr, nullptr, Prec_None },
    [TokenType_Void]             = { nullptr, nullptr, Prec_None },
    [TokenType_TokenTypeCount]   = { nullptr, nullptr, Prec_None },
};

static P_ParseRule* P_GetRule(L_TokenType type) {
    return &parse_rules[type];
}

static P_Expr* P_ExprPrecedence(P_Parser* parser, P_Precedence precedence) {
    P_Advance(parser);
    P_PrefixParseFn prefix_rule = P_GetRule(parser->previous.type)->prefix;
    if (prefix_rule == nullptr) {
        report_error(parser, str_lit("Expected Expression\n"));
        return nullptr;
    }
    
    P_Expr* e = prefix_rule(parser);
    while (precedence <= P_GetRule(parser->current.type)->precedence) {
        P_Advance(parser);
        P_InfixParseFn infix = P_GetRule(parser->previous.type)->infix;
        e = infix(parser, e);
    }
    return e;
}

static P_Expr* P_Expression(P_Parser* parser) {
    return P_ExprPrecedence(parser, Prec_Assignment);
}

//~ Statements
static P_Stmt* P_Statement(P_Parser* parser);
static P_Stmt* P_Declaration(P_Parser* parser);

static P_Stmt* P_StmtFuncDecl(P_Parser* parser, P_ValueType type, string name) {
    P_ValueType test;
    // NOTE(voxel): Parse Parameters
    P_ValueType params[256];
    string param_names[256];
    u32 arity = 0;
    
    P_ValueType prev_function_body_ret = parser->function_body_ret;
    b8 prev_directly_in_func_body = parser->is_directly_in_func_body;
    
    parser->all_code_paths_return = false;
    parser->encountered_return = false;
    parser->function_body_ret = type;
    parser->is_directly_in_func_body = true;
    
    parser->scope_depth++;
    
    while (!P_Match(parser, TokenType_CloseParenthesis)) {
        P_ConsumeType(parser, str_lit("Expected type\n"));
        params[arity] = P_TypeTokenToValueType(parser);
        P_Consume(parser, TokenType_Identifier, str_lit("Expected param name\n"));
        string param_name = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
        param_names[arity] = param_name;
        var_hash_table_set(&parser->variables, (var_entry_key) { .name = param_name, .depth = parser->scope_depth }, params[arity]);
        
        arity++;
        if (!P_Match(parser, TokenType_Comma)) {
            P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after parameters\n"));
            break;
        }
    }
    
    string actual_name = P_FuncNameMangle(parser, name, arity, params, (string){0});
    if (str_eq(actual_name, str_lit("main_0")))
        actual_name = str_lit("main");
    
    // TODO(voxel) There should be an arena_push function for this cuz i don't like using memcpy in user code
    P_ValueType* param_buffer = arena_alloc(&parser->arena, arity * sizeof(P_ValueType));
    memcpy(param_buffer, params, arity * sizeof(P_ValueType));
    string* param_names_buffer = arena_alloc(&parser->arena, arity * sizeof(string));
    memcpy(param_names_buffer, param_names, arity * sizeof(string));
    
    func_entry_key key = (func_entry_key) { .name = actual_name, .depth = parser->scope_depth - 1 };
    if (!func_hash_table_get(&parser->functions, key, &test)) {
        func_hash_table_set(&parser->functions, key, type);
    } else report_error(parser, str_lit("Cannot redeclare function %.*s\n"), name.size, name.str);
    
    // Block Stuff
    P_Consume(parser, TokenType_OpenBrace, str_lit("Expected {\n"));
    
    P_Stmt* func = P_MakeFuncStmtNode(parser, type, actual_name, arity, param_buffer, param_names_buffer);
    
    P_Stmt* curr = nullptr;
    while (!P_Match(parser, TokenType_CloseBrace)) {
        P_Stmt* stmt = P_Declaration(parser);
        
        if (curr == nullptr) func->op.func_decl.block = stmt;
        else curr->next = stmt;
        
        curr = stmt;
        if (parser->current.type == TokenType_EOF)
            report_error(parser, str_lit("Unterminated Function Block. Expected }\n"));
    }
    // Pop all entries with the current scope depth from variables here
    for (u32 i = 0; i < parser->variables.capacity; i++) {
        if (parser->variables.entries[i].key.depth == parser->scope_depth)
            var_hash_table_del(&parser->variables, parser->variables.entries[i].key);
    }
    
    parser->scope_depth--;
    parser->function_body_ret = prev_function_body_ret;
    parser->is_directly_in_func_body = prev_directly_in_func_body;
    if (!parser->all_code_paths_return) report_error(parser, str_lit("Not all code paths return a value\n"));
    return func;
}

static P_Stmt* P_StmtVarDecl(P_Parser* parser, P_ValueType type, string name) {
    P_ValueType test;
    var_entry_key key = (var_entry_key) { .name = name, .depth = parser->scope_depth };
    if (!var_hash_table_get(&parser->variables, key, &test)) {
        var_hash_table_set(&parser->variables, key, type);
    }
    else report_error(parser, str_lit("Cannot redeclare variable %.*s\n"), name.size, name.str);
    
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected semicolon after name\n"));
    return P_MakeVarDeclStmtNode(parser, type, name);
}

static P_Stmt* P_StmtStructureDecl(P_Parser* parser) {
    P_Consume(parser, TokenType_Identifier, str_lit("Expected Struct name after keyword 'struct'\n"));
    string name = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
    
    if (structure_exists(parser, name, parser->scope_depth))
        report_error(parser, str_lit("Cannot redeclare structure with name %.*s\n"), name.size, name.str);
    P_Consume(parser, TokenType_OpenBrace, str_lit("Expected { after Struct Name\n"));
    
    u64 idx = parser->structures.count;
    struct_array_add(&parser->structures, (P_Struct) { .name = name, .depth = parser->scope_depth });
    u32 member_count = 0;
    string member_names[1024];
    P_ValueType member_types[1024];
    
    while (!P_Match(parser, TokenType_CloseBrace)) {
        P_ConsumeType(parser, str_lit("Expected Type or }"));
        member_types[member_count] = P_TypeTokenToValueType(parser);
        P_Consume(parser, TokenType_Identifier, str_lit("Expected member name"));
        member_names[member_count] = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
        P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after name"));
        member_count++;
        
        if (parser->current.type == TokenType_EOF)
            report_error(parser, str_lit("Unterminated block for structure definition"));
    }
    
    P_ValueType* member_types_buffer = arena_alloc(&parser->arena, member_count * sizeof(P_ValueType));
    memcpy(member_types_buffer, member_types, member_count * sizeof(P_ValueType));
    string* member_names_buffer = arena_alloc(&parser->arena, member_count * sizeof(string));
    memcpy(member_names_buffer, member_names, member_count * sizeof(string));
    
    parser->structures.elements[idx].member_count = member_count;
    parser->structures.elements[idx].member_types = member_types;
    parser->structures.elements[idx].member_names = member_names;
    
    return P_MakeStructDeclStmtNode(parser, name, member_count, member_types_buffer, member_names_buffer);
}

static P_Stmt* P_StmtBlock(P_Parser* parser) {
    b8 prev_directly_in_func_body = parser->is_directly_in_func_body;
    parser->is_directly_in_func_body = false;
    parser->scope_depth++;
    P_Stmt* block = P_MakeBlockStmtNode(parser);
    P_Stmt* curr = nullptr;
    while (!P_Match(parser, TokenType_CloseBrace)) {
        P_Stmt* stmt = P_Declaration(parser);
        
        if (curr == nullptr) block->op.block = stmt;
        else curr->next = stmt;
        
        curr = stmt;
        if (parser->current.type == TokenType_EOF)
            report_error(parser, str_lit("Unterminated Block statement. Expected }\n"));
    }
    // Pop all entries with the current scope depth from variables here
    for (u32 i = 0; i < parser->variables.capacity; i++) {
        if (parser->variables.entries[i].key.depth == parser->scope_depth)
            var_hash_table_del(&parser->variables, parser->variables.entries[i].key);
    }
    parser->scope_depth--;
    parser->is_directly_in_func_body = prev_directly_in_func_body;
    return block;
}

static P_Stmt* P_StmtReturn(P_Parser* parser) {
    parser->encountered_return = true;
    if (parser->is_directly_in_func_body)
        parser->all_code_paths_return = true;
    
    P_Expr* val = P_Expression(parser);
    if (val == nullptr) return nullptr;
    if (!str_eq(val->ret_type, parser->function_body_ret))
        report_error(parser, str_lit("Function return type mismatch. Expected %.*s\n"), parser->function_body_ret.size, parser->function_body_ret.str);
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after return statement\n"));
    return P_MakeReturnStmtNode(parser, val);
}

static P_Stmt* P_StmtIf(P_Parser* parser) {
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected ( after if\n"));
    P_Expr* condition = P_Expression(parser);
    if (!str_eq(condition->ret_type, ValueType_Bool)) {
        report_error(parser, str_lit("If statement condition doesn't resolve to boolean\n"));
    }
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression"));
    
    b8 reset = false;
    b8 prev_directly_in_func_body = parser->is_directly_in_func_body;
    if (parser->current.type != TokenType_OpenBrace) {
        parser->is_directly_in_func_body = false;
        parser->scope_depth++;
        reset = true;
    }
    
    P_Stmt* then = P_Statement(parser);
    if (P_Match(parser, TokenType_Else)) {
        P_Stmt* else_s = P_Statement(parser);
        return P_MakeIfElseStmtNode(parser, condition, then, else_s);
    }
    
    if (reset) {
        parser->scope_depth--;
        parser->is_directly_in_func_body = prev_directly_in_func_body;
    }
    
    return P_MakeIfStmtNode(parser, condition, then);
}

static P_Stmt* P_StmtWhile(P_Parser* parser) {
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected ( after while\n"));
    P_Expr* condition = P_Expression(parser);
    if (!str_eq(condition->ret_type, ValueType_Bool)) {
        report_error(parser, str_lit("While loop condition doesn't resolve to boolean\n"));
    }
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression"));
    
    b8 reset = false;
    b8 prev_directly_in_func_body = parser->is_directly_in_func_body;
    if (parser->current.type != TokenType_OpenBrace) {
        parser->is_directly_in_func_body = false;
        parser->scope_depth++;
        reset = true;
    }
    
    P_Stmt* then = P_Statement(parser);
    
    if (reset) {
        parser->scope_depth--;
        parser->is_directly_in_func_body = prev_directly_in_func_body;
    }
    
    return P_MakeWhileStmtNode(parser, condition, then);
}

static P_Stmt* P_StmtDoWhile(P_Parser* parser) {
    b8 reset = false;
    b8 prev_directly_in_func_body = parser->is_directly_in_func_body;
    if (parser->current.type != TokenType_OpenBrace) {
        parser->is_directly_in_func_body = false;
        parser->scope_depth++;
        reset = true;
    }
    
    P_Stmt* then = P_Statement(parser);
    P_Consume(parser, TokenType_While, str_lit("Expected 'while'"));
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected ( after while\n"));
    P_Expr* condition = P_Expression(parser);
    if (!str_eq(condition->ret_type, ValueType_Bool)) {
        report_error(parser, str_lit("While loop condition doesn't resolve to boolean\n"));
    }
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression"));
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after )"));
    
    if (reset) {
        parser->scope_depth--;
        parser->is_directly_in_func_body = prev_directly_in_func_body;
    }
    
    return P_MakeDoWhileStmtNode(parser, condition, then);
}

static P_Stmt* P_StmtExpression(P_Parser* parser) {
    P_Expr* expr = P_Expression(parser);
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after expression\n"));
    return P_MakeExpressionStmtNode(parser, expr);
}

static P_Stmt* P_Statement(P_Parser* parser) {
    if (!str_eq(parser->function_body_ret, ValueType_Invalid)) {
        if (P_Match(parser, TokenType_OpenBrace))
            return P_StmtBlock(parser);
        if (P_Match(parser, TokenType_Return))
            return P_StmtReturn(parser);
        if (P_Match(parser, TokenType_If))
            return P_StmtIf(parser);
        if (P_Match(parser, TokenType_While))
            return P_StmtWhile(parser);
        if (P_Match(parser, TokenType_Do))
            return P_StmtDoWhile(parser);
        return P_StmtExpression(parser);
    }
    report_error(parser, str_lit("Cannot Have Statements that exist outside of functions\n"));
    return nullptr;
}

static P_Stmt* P_Declaration(P_Parser* parser) {
    P_Stmt* s;
    
    if (P_IsTypeToken(parser)) {
        P_ValueType type = P_TypeTokenToValueType(parser);
        P_Consume(parser, TokenType_Identifier, str_lit("Expected Identifier after variable type\n"));
        
        string name = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
        if (P_Match(parser, TokenType_OpenParenthesis)) {
            s = P_StmtFuncDecl(parser, type, name);
        } else {
            s = P_StmtVarDecl(parser, type, name);
        }
    } else if (P_Match(parser, TokenType_Struct)) {
        s = P_StmtStructureDecl(parser);
    } else
        s = P_Statement(parser);
    
    P_Sync(parser);
    return s;
}

//~ API
void P_Parse(P_Parser* parser) {
    P_Advance(parser);
    parser->root = P_Declaration(parser);
    P_Stmt* c = parser->root;
    while (parser->current.type != TokenType_EOF && !parser->had_error) {
        c->next = P_Declaration(parser);
        c = c->next;
    }
    
    P_ValueType v;
    if (!func_hash_table_get(&parser->functions, (func_entry_key) { .name = str_lit("main"), .depth = 0 }, &v)) {
        report_error(parser, str_lit("No main function definition found\n"));
    }
}

void P_Initialize(P_Parser* parser, string source) {
    arena_init(&parser->arena);
    L_Initialize(&parser->lexer, source);
    parser->root = nullptr;
    parser->current = (L_Token) {0};
    parser->previous = (L_Token) {0};
    parser->had_error = false;
    parser->panik_mode = false;
    parser->scope_depth = 0;
    parser->function_body_ret = ValueType_Invalid;
    parser->is_directly_in_func_body = false;
    parser->encountered_return = false;
    parser->all_code_paths_return = false;
    var_hash_table_init(&parser->variables);
    func_hash_table_init(&parser->functions);
    struct_array_init(&parser->structures);
}

void P_Free(P_Parser* parser) {
    var_hash_table_free(&parser->variables);
    func_hash_table_free(&parser->functions);
    struct_array_free(&parser->structures);
    arena_free(&parser->arena);
}

static void P_PrintExprAST_Indent(M_Arena* arena, P_Expr* expr, u8 indent) {
    for (u8 i = 0; i < indent; i++)
        printf("  ");
    
    switch (expr->type) {
        case ExprType_IntLit: {
            printf("%d [Integer]\n", expr->op.integer_lit);
        } break;
        
        case ExprType_LongLit: {
            printf("%I64d [Long]\n", expr->op.long_lit);
        } break;
        
        case ExprType_FloatLit: {
            printf("%f [Float]\n", expr->op.float_lit);
        } break;
        
        case ExprType_DoubleLit: {
            printf("%f [Double]\n", expr->op.double_lit);
        } break;
        
        case ExprType_StringLit: {
            printf("%.*s [String]\n", (int)expr->op.string_lit.size, expr->op.string_lit.str);
        } break;
        
        case ExprType_CharLit: {
            printf("%.*s [Char]\n", (int)expr->op.char_lit.size, expr->op.char_lit.str);
        } break;
        
        case ExprType_BoolLit: {
            printf("%s [Bool]\n", expr->op.bool_lit ? "True" : "False");
        } break;
        
        case ExprType_Assignment: {
            P_PrintExprAST_Indent(arena, expr->op.assignment.name, indent + 1);
            printf("=\n");
            P_PrintExprAST_Indent(arena, expr->op.assignment.value, indent + 1);
        } break;
        
        case ExprType_Variable: {
            printf("%.*s\n", (int)expr->op.variable.size, expr->op.variable.str);
        } break;
        
        case ExprType_Unary: {
            printf("%s\n", L__get_string_from_type__(expr->op.unary.operator).str);
            P_PrintExprAST_Indent(arena, expr->op.unary.operand, indent + 1);
        } break;
        
        case ExprType_Binary: {
            printf("%s\n", L__get_string_from_type__(expr->op.binary.operator).str);
            P_PrintExprAST_Indent(arena, expr->op.binary.left, indent + 1);
            P_PrintExprAST_Indent(arena, expr->op.binary.right, indent + 1);
        } break;
        
        case ExprType_FuncCall: {
            printf("%s()\n", expr->op.func_call.name.str);
            for (u32 i = 0; i < expr->op.func_call.call_arity; i++)
                P_PrintExprAST_Indent(arena, expr->op.func_call.params[i], indent + 1);
        } break;
        
        case ExprType_Dot: {
            printf(".%.*s [Member Access]\n", (i32)expr->op.dot.right.size, expr->op.dot.right.str);
            P_PrintExprAST_Indent(arena, expr->op.dot.left, indent + 1);
        } break;
    }
}

void P_PrintExprAST(P_Expr* expr) {
    M_Arena arena;
    arena_init(&arena);
    P_PrintExprAST_Indent(&arena, expr, 0);
    arena_free(&arena);
}

static void P_PrintAST_Indent(M_Arena* arena, P_Stmt* stmt, u8 indent) {
    for (u8 i = 0; i < indent; i++)
        printf("  ");
    
    switch (stmt->type) {
        case StmtType_Block: {
            printf("Block Statement:\n");
            if (stmt->op.block != nullptr)
                P_PrintAST_Indent(arena, stmt->op.block, indent + 1);
        } break;
        
        case StmtType_Expression: {
            printf("Expression Statement:\n");
            P_PrintExprAST_Indent(arena, stmt->op.expression, indent + 1);
        } break;
        
        case StmtType_If: {
            printf("If Statement:\n");
            P_PrintExprAST_Indent(arena, stmt->op.if_s.condition, indent + 1);
            for (u8 i = 0; i < indent; i++)
                printf("  ");
            printf("Then\n");
            P_PrintAST_Indent(arena, stmt->op.if_s.then, indent + 1);
        } break;
        
        case StmtType_IfElse: {
            printf("If Statement:\n");
            P_PrintExprAST_Indent(arena, stmt->op.if_else.condition, indent + 1);
            for (u8 i = 0; i < indent; i++)
                printf("  ");
            printf("Then\n");
            P_PrintAST_Indent(arena, stmt->op.if_else.then, indent + 1);
            for (u8 i = 0; i < indent; i++)
                printf("  ");
            printf("Else\n");
            P_PrintAST_Indent(arena, stmt->op.if_else.else_s, indent + 1);
        } break;
        
        case StmtType_Return: {
            printf("Return Statement:\n");
            P_PrintExprAST_Indent(arena, stmt->op.returned, indent + 1);
        } break;
        
        case StmtType_VarDecl: {
            printf("Variable Declaration:\n");
            for (u8 i = 0; i < indent; i++)
                printf("  ");
            printf("%.*s: %s\n", (i32)stmt->op.var_decl.name.size, stmt->op.var_decl.name.str, stmt->op.var_decl.type.str);
        } break;
        
        case StmtType_FuncDecl: {
            printf("Function Declaration: %s: %s\n", stmt->op.func_decl.name.str, stmt->op.func_decl.type.str);
            if (stmt->op.func_decl.block != nullptr)
                P_PrintAST_Indent(arena, stmt->op.func_decl.block, indent + 1);
        } break;
        
        case StmtType_StructDecl: {
            printf("Struct Declaration: %s\n", stmt->op.struct_decl.name.str);
            for (u32 i = 0; i < stmt->op.struct_decl.member_count; i++) {
                for (u8 idt = 0; idt < indent + 1; idt++)
                    printf("  ");
                printf("%.*s: %s\n", (i32)stmt->op.struct_decl.member_names[i].size, stmt->op.struct_decl.member_names[i].str, stmt->op.struct_decl.member_types[i].str);
            }
        } break;
        
        case StmtType_While: {
            printf("While Loop:\n");
            P_PrintExprAST_Indent(arena, stmt->op.while_s.condition, indent + 1);
            P_PrintAST_Indent(arena, stmt->op.while_s.then, indent + 1);
        } break;
        
        case StmtType_DoWhile: {
            printf("Do-While Loop:\n");
            P_PrintExprAST_Indent(arena, stmt->op.do_while.condition, indent + 1);
            P_PrintAST_Indent(arena, stmt->op.do_while.then, indent + 1);
        } break;
    }
    
    if (stmt->next != nullptr)
        P_PrintAST_Indent(arena, stmt->next, indent);
}

void P_PrintAST(P_Stmt* stmt) {
    M_Arena arena;
    arena_init(&arena);
    P_PrintAST_Indent(&arena, stmt, 0);
    arena_free(&arena);
}
