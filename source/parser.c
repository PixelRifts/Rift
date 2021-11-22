#include "parser.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef CPCOM_WIN
#include <direct.h>
#define get_cwd _getcwd
#define PATH_MAX 4096
#endif
// @linux Add linux branch here

#include "operator_bindings.h"
#include "types.h"

//~ Globals
static M_Arena global_arena;
static var_hash_table variables;
static func_hash_table functions;
static string_list imports;
static string_list imports_parsing;
static type_array types;
static string cwd;


static string fix_filepath(M_Arena* arena, string path) {
    string fixed = path;
    while (true) {
        u64 dotdot = str_find_first(fixed, str_lit(".."), 0);
        if (dotdot == fixed.size) break;
        
        u64 last_slash = str_find_last(fixed, str_lit("/"), dotdot-1);
        
        u64 range = (dotdot + 3) - last_slash;
        string old = fixed;
        fixed = str_alloc(arena, fixed.size - range);
        memcpy(fixed.str, old.str, last_slash);
        memcpy(fixed.str + last_slash, old.str + dotdot + 3, old.size - range - last_slash);
    }
    fixed = str_replace_all(arena, fixed, str_lit("./"), (string) {0});
    return fixed;
}

//~ Errors
static void report_error_at(P_Parser* parser, L_Token* token, string message, ...) {
    if (parser->panik_mode) exit(1);
    
    fprintf(stderr, "Error:%.*s:%d: ", str_expand(parser->filename), token->line);
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
static P_Container* type_array_get(P_Parser* parser, string struct_name, u32 depth) {
    for (u32 i = 0; i < types.count; i++) {
        if (str_eq(struct_name, types.elements[i].name) && types.elements[i].depth <= depth)
            return &types.elements[i];
    }
    return nullptr;
}

static b8 container_type_exists(P_Parser* parser, string struct_name, u32 depth) {
    for (u32 i = 0; i < types.count; i++) {
        if (str_eq(struct_name, types.elements[i].name) && types.elements[i].depth <= depth)
            return true;
    }
    return false;
}

static P_ValueType member_type_get(P_Container* structure, string reqd) {
    string_list_node* curr = structure->member_names.first;
    value_type_list_node* type = structure->member_types.first;
    for (u32 i = 0; i < structure->member_count; i++) {
        
        if (curr->size == reqd.size) {
            if (memcmp(curr->str, reqd.str, curr->size) == 0) {
                return (P_ValueType) {
                    .base_type = type->type.base_type,
                    .full_type = type->type.full_type,
                    .mods = nullptr
                };
            }
        }
        curr = curr->next;
        type = type->next;
    }
    return ValueType_Invalid;
}

static b8 member_exists(P_Container* structure, string reqd) {
    string_list_node* curr = structure->member_names.first;
    for (u32 i = 0; i < structure->member_count; i++) {
        if (curr->size == reqd.size) {
            if (memcmp(curr->str, reqd.str, curr->size) == 0)
                return true;
        }
        curr = curr->next;
    }
    return false;
}

void P_Advance(P_Parser* parser) {
    parser->previous_two = parser->previous;
    parser->previous = parser->current;
    parser->current = parser->next;
    parser->next = parser->next_two;
    
    while (true) {
        parser->next_two = L_LexToken(&parser->lexer);
        if (parser->next_two.type != TokenType_Error) break;
        report_error_at_current(parser, (string) { .str = (u8*)parser->current.start, .size = parser->current.length });
    }
}

static void P_Consume(P_Parser* parser, L_TokenType type, string message) {
    if (parser->current.type == type) { P_Advance(parser); return; }
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
    if (parser->current.type == TokenType_Int    ||
        parser->current.type == TokenType_Long   ||
        parser->current.type == TokenType_Float  ||
        parser->current.type == TokenType_Double ||
        parser->current.type == TokenType_Char   ||
        parser->current.type == TokenType_Bool   ||
        parser->current.type == TokenType_Void   ||
        parser->current.type == TokenType_String) return true;
    
    if (parser->current.type == TokenType_Hat) {
        // This is for lambda syntax
        if (parser->next.type == TokenType_OpenParenthesis)
            return false;
        else return true;
    }
    
    if (parser->current.type == TokenType_Identifier) {
        if (container_type_exists(parser, (string) { .str = parser->current.start, .size = parser->current.length }, parser->scope_depth)) {
            return true;
        }
    }
    return false;
}

static P_ValueType P_ReduceType(P_ValueType in) {
    if (in.mod_ct == 0) return in;
    in.mod_ct--;
    switch (in.mods[in.mod_ct].type) {
        case ValueTypeModType_Pointer: {
            in.full_type.size--;
        } break;
        
        case ValueTypeModType_Array: {
            in.full_type.size -= str_find_last(in.full_type, str_lit("["), 0)-1;
        } break;
        
        case ValueTypeModType_Reference: {
            in.full_type.size--;
        } break;
        
        default: {
            exit(REDUCTION_ERROR);
        } break;
    }
    return in;
}

static P_Expr* P_MakeIntNode(P_Parser* parser, i32 value);

static P_ValueType P_PushPointerType(P_Parser* parser, P_ValueType in) {
    P_ValueType out = in;
    out.full_type = str_cat(&parser->arena, in.full_type, str_lit("*"));
    P_ValueTypeMod* new_mods = arena_alloc(&parser->arena, sizeof(P_ValueTypeMod) * (in.mod_ct + 1));
    memmove(new_mods, in.mods, sizeof(P_ValueTypeMod) * in.mod_ct);
    out.mods = new_mods;
    out.mods[in.mod_ct] = (P_ValueTypeMod) { .type = ValueTypeModType_Pointer };
    out.mod_ct = in.mod_ct + 1;
    return out;
}

static P_ValueType P_PushArrayType(P_Parser* parser, P_ValueType in, u32 count) {
    P_ValueType out = in;
    out.full_type = str_cat(&parser->arena, in.full_type, str_from_format(&parser->arena, "[%u]", count));
    P_ValueTypeMod* new_mods = arena_alloc(&parser->arena, sizeof(P_ValueTypeMod) * (in.mod_ct + 1));
    memmove(new_mods, in.mods, sizeof(P_ValueTypeMod) * in.mod_ct);
    
    out.mods = new_mods;
    out.mods[in.mod_ct] = (P_ValueTypeMod) { .type = ValueTypeModType_Array };
    out.mods[in.mod_ct].op.array_expr = P_MakeIntNode(parser, (i32)count);
    out.mod_ct = in.mod_ct + 1;
    return out;
}

static P_ValueType P_CreateFnpointerType(P_Parser* parser, P_ValueType* return_type, value_type_list* params) {
    P_ValueType out = {0};
    out.type = ValueTypeType_FuncPointer;
    out.full_type = str_cat(&parser->arena, out.full_type, return_type->full_type);
    out.full_type = str_cat(&parser->arena, out.full_type, str_lit("("));
    value_type_list_node* curr = params->first;
    while (curr != nullptr) {
        out.full_type = str_cat(&parser->arena, out.full_type, curr->type.full_type);
        curr = curr->next;
    }
    out.full_type = str_cat(&parser->arena, out.full_type, str_lit(")"));
    out.op.func_ptr.ret_type = return_type;
    out.op.func_ptr.func_param_types = params;
    return out;
}

// Just for mod consumption
static P_Expr* P_Expression(P_Parser* parser);

static b8 P_ConsumeTypeMods(P_Parser* parser, type_mod_array* mods) {
    if (P_Match(parser, TokenType_Star)) {
        type_mod_array_add(&parser->arena, mods, (P_ValueTypeMod) { .type = ValueTypeModType_Pointer });
        P_ConsumeTypeMods(parser, mods);
        return true;
    } else if (P_Match(parser, TokenType_OpenBracket)) {
        P_Expr* e = P_Expression(parser);
        
        P_Consume(parser, TokenType_CloseBracket, str_lit("Required ] after expression in type declaration\n"));
        type_mod_array_add(&parser->arena, mods, (P_ValueTypeMod) { .type = ValueTypeModType_Array, .op.array_expr = e });
        P_ConsumeTypeMods(parser, mods);
        return true;
    } else if (P_Match(parser, TokenType_Ampersand)) {
        type_mod_array_add(&parser->arena, mods, (P_ValueTypeMod) { .type = ValueTypeModType_Reference });
        P_ConsumeTypeMods(parser, mods);
        return true;
    }
    
    return false;
}

static P_ValueType P_ConsumeType(P_Parser* parser, string message);

static b8 P_MatchType(P_Parser* parser, P_ValueType* rettype, string* custom_err) {
    if (parser->current.type == TokenType_Int || parser->current.type == TokenType_Long ||
        parser->current.type == TokenType_Float || parser->current.type == TokenType_Double ||
        parser->current.type == TokenType_Bool || parser->current.type == TokenType_Char ||
        parser->current.type == TokenType_Void || parser->current.type == TokenType_String ||
        parser->current.type == TokenType_Identifier) {
        if (parser->current.type == TokenType_Identifier) {
            if (container_type_exists(parser, (string) { .str = parser->current.start, .size = parser->current.length }, parser->scope_depth)) {
                P_Advance(parser);
            } else {
                *rettype = ValueType_Invalid;
                return false;
            }
        } else {
            P_Advance(parser);
        }
        
        string base_type = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
        type_mod_array mods = {0};
        P_ConsumeTypeMods(parser, &mods);
        u64 complete_length = ((u64)parser->previous.start + (u64)parser->previous.length) - (u64)base_type.str;
        
        *rettype = (P_ValueType) {
            .type = ValueTypeType_Basic,
            .base_type = base_type,
            .full_type = (string) { .str = base_type.str, .size = complete_length },
            .mods = mods.elements,
            .mod_ct = mods.count,
        };
        return true;
    } else if (P_Match(parser, TokenType_Hat)) {
        P_ValueType* ret_type = arena_alloc(&parser->arena, sizeof(P_ValueType));
        string base_type = (string) { .str = (u8*)parser->current.start, .size = parser->current.length };
        *ret_type = P_ConsumeType(parser, str_lit("Expected Type as return type for Function Pointer\n"));
        P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected (\n"));
        
        type_mod_array func_mods = {0};
        value_type_list* list = nullptr;
        
        list = arena_alloc(&parser->arena, sizeof(value_type_list));
        while (!P_Match(parser, TokenType_CloseParenthesis)) {
            if (P_Match(parser, TokenType_EOF)) {
                report_error(parser, str_lit("Unclosed Function Pointer Parenthesis\n"));
                *rettype = ValueType_Invalid;
                return false;
            }
            P_ValueType type = P_ConsumeType(parser, str_lit("Expected Type in Function Pointer declaration\n"));
            value_type_list_push(&parser->arena, list, type);
            
            if (!P_Match(parser, TokenType_Comma)) {
                P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected , or )\n"));
                break;
            }
        }
        P_ConsumeTypeMods(parser, &func_mods);
        u64 complete_length = ((u64)parser->previous.start + (u64)parser->previous.length) - (u64)base_type.str;
        
        *rettype = (P_ValueType) {
            .type = ValueTypeType_FuncPointer,
            .base_type = base_type,
            .full_type = (string) { .str = base_type.str, .size = complete_length },
            .mods = func_mods.elements,
            .mod_ct = func_mods.count,
            
            .op.func_ptr.ret_type = ret_type,
            .op.func_ptr.func_param_types = list,
        };
        
        return true;
    }
    *rettype = ValueType_Invalid;
    return false;
}

static P_ValueType P_ConsumeType(P_Parser* parser, string message) {
    P_ValueType type;
    string error = {0};
    if (!P_MatchType(parser, &type, &error))
        report_error_at_current(parser, message);
    if (error.size != 0)
        report_error(parser, error);
    return type;
}

static P_ValueType type_heirarchy[] = {
    value_type_abs("double"),
    value_type_abs("float"),
    value_type_abs("long"),
    value_type_abs("int"),
    value_type_abs("char"),
};
u32 type_heirarchy_length = 5;

b8 type_check(P_ValueType a, P_ValueType expected) {
    if (str_eq(a.full_type, expected.full_type)) return true;
    
    if ((a.type == ValueTypeType_FuncPointer) && (expected.type == ValueTypeType_FuncPointer)) {
        if (!type_check(*a.op.func_ptr.ret_type, *expected.op.func_ptr.ret_type))
            return false;
        
        value_type_list_node* curr_test = expected.op.func_ptr.func_param_types->first;
        value_type_list_node* curr = a.op.func_ptr.func_param_types->first;
        while (!(curr_test == nullptr || curr == nullptr)) {
            
            if (!str_eq(str_lit("..."), curr_test->type.full_type)) {
                // Only looks for absolute matches. No fuzz
                if (!str_eq(curr->type.full_type, curr_test->type.full_type))
                    break;
                curr_test = curr_test->next;
                curr = curr->next;
            } else {
                if (curr == a.op.func_ptr.func_param_types->last) {
                    curr = curr->next;
                    curr_test = curr_test->next;
                } else curr = curr->next;
            }
            
        }
        
        if (curr_test == nullptr && curr == nullptr) {
            return true;
        }
        
    } else if (a.type == ValueTypeType_FuncPointer || expected.type == ValueTypeType_FuncPointer) {
        return false;
    }
    
    // Test for 'invalid' type
    if (str_eq(a.full_type, ValueType_Invalid.full_type) ||
        str_eq(expected.full_type, ValueType_Invalid.full_type)) {
        return false;
    }
    
    // Test for 'any' type
    if (str_eq(a.full_type, ValueType_Any.full_type) ||
        str_eq(expected.full_type, ValueType_Any.full_type)) {
        return true;
    }
    
    // Ref checking
    if (is_ref(&expected)) {
        P_ValueType o = P_ReduceType(expected);
        return type_check(a, o);
    }
    
    // Array testing [ any[] -> any* ]
    if (is_array(&a)) {
        if (is_ptr(&expected)) {
            return true;
        }
    }
    
    // Pointer testing [ any* -> void* ]
    if (str_eq(expected.base_type, ValueType_Void.base_type)) {
        if (is_ptr(&expected)) {
            if (is_ptr(&a)) {
                return true;
            }
        }
    }
    // Pointer testing [ void* -> any* ]
    if (str_eq(a.base_type, ValueType_Void.base_type)) {
        if (is_ptr(&a)) {
            if (is_ptr(&expected)) {
                return true;
            }
        }
    }
    
    // Implicit cast thingy
    i32 perm = -1;
    for (u32 i = 0; i < type_heirarchy_length; i++) {
        if (str_eq(type_heirarchy[i].base_type, expected.full_type)) {
            perm = i;
            break;
        }
    }
    if (perm != -1) {
        i32 other = -1;
        for (u32 i = 0; i < type_heirarchy_length; i++) {
            if (str_eq(type_heirarchy[i].base_type, a.full_type)) {
                other = i;
                break;
            }
        }
        return perm < other;
    }
    
    return false;
}

static string P_FuncNameMangle(P_Parser* parser, string name, u32 arity, value_type_list params, string additional_info) {
    
    string_list sl = {0};
    string_list_push(&parser->arena, &sl, str_from_format(&parser->arena, "%.*s_%u", str_expand(name), arity));
    
    value_type_list_node* curr = params.first;
    for (u32 i = 0; i < arity; i++) {
        string fixed = str_replace_all(&parser->arena,
                                       str_from_format(&parser->arena, "%.*s", str_expand(curr->type.full_type)),
                                       str_lit("*"), str_lit("ptr"));
        fixed = str_replace_all(&parser->arena, fixed, str_lit("&"), str_lit("ref"));
        
        string_list_push(&parser->arena, &sl, fixed);
        curr = curr->next;
    }
    
    if (additional_info.size != 0) string_list_push(&parser->arena, &sl, str_from_format(&parser->arena, "_%s", additional_info.str));
    return string_list_flatten(&parser->arena, &sl);
}

static b8 P_IsInScope(P_Parser* parser, P_ScopeType type) {
    u32 scope_idx = parser->scopetype_tos - 1;
    while (scope_idx != -1) {
        if (parser->scopetype_stack[scope_idx] == type)
            return true;
        scope_idx--;
    }
    return false;
}

//~ Binding
static b8 P_CheckValueType(P_ValueTypeCollection expected, P_ValueType type) {
    if (expected == ValueTypeCollection_Number) {
        return (type_check(type, ValueType_Integer) || type_check(type, ValueType_Long) || type_check(type, ValueType_Float) || type_check(type, ValueType_Double));
    } else if (expected == ValueTypeCollection_Pointer) {
        return is_ptr(&type);
    } else if (expected == ValueTypeCollection_WholeNumber) {
        return type_check(type, ValueType_Integer) || type_check(type, ValueType_Long);
    } else if (expected == ValueTypeCollection_DecimalNumber) {
        return type_check(type, ValueType_Double) || type_check(type, ValueType_Float);
    } else if (expected == ValueTypeCollection_String) {
        return type_check(type, ValueType_String);
    } else if (expected == ValueTypeCollection_Char) {
        return type_check(type, ValueType_Char);
    } else if (expected == ValueTypeCollection_Bool) {
        return type_check(type, ValueType_Bool);
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
    report_error(parser, error_msg, expr->ret_type.full_type);
}

static void P_BindDouble(P_Parser* parser, P_Expr* left, P_Expr* right, P_BinaryOpPair* expected, u32 flagct, string error_msg) {
    for (u32 i = 0; i < flagct; i++) {
        if (P_CheckValueType(expected[i].left, left->ret_type) && P_CheckValueType(expected[i].right, right->ret_type)) {
            return;
        }
    }
    report_error(parser, error_msg, left->ret_type.full_type, right->ret_type.full_type);
}

static P_ValueType P_GetNumberBinaryValType(P_Expr* a, P_Expr* b) {
    if (is_ptr(&a->ret_type)) {
        return a->ret_type;
    } else if (is_ptr(&b->ret_type)) {
        return b->ret_type;
    } else {
        if (str_eq(a->ret_type.full_type, ValueType_Double.full_type) || str_eq(b->ret_type.full_type, ValueType_Double.full_type))
            return ValueType_Double;
        if (str_eq(a->ret_type.full_type, ValueType_Float.full_type) || str_eq(b->ret_type.full_type, ValueType_Float.full_type))
            return ValueType_Float;
        if (str_eq(a->ret_type.full_type, ValueType_Long.full_type) || str_eq(b->ret_type.full_type, ValueType_Long.full_type))
            return ValueType_Long;
        if (str_eq(a->ret_type.full_type, ValueType_Integer.full_type) || str_eq(b->ret_type.full_type, ValueType_Integer.full_type))
            return ValueType_Integer;
        if (str_eq(a->ret_type.full_type, ValueType_Char.full_type) || str_eq(b->ret_type.full_type, ValueType_Char.full_type))
            return ValueType_Char;
    }
    return ValueType_Invalid;
}

static b8 P_GetVariable(P_Parser* parser, string name, P_ValueType* type) {
    if (parser->is_in_private_scope) {
        var_entry_key key = { .name = name, .depth = parser->scope_depth };
        if (var_hash_table_get(&variables, key, type)) {
            return true;
        }
    } else {
        var_entry_key key = { .name = name, .depth = parser->scope_depth };
        while (key.depth != -1) {
            if (var_hash_table_get(&variables, key, type)) {
                return true;
            }
            key.depth++;
        }
    }
    return false;
}

static P_ScopeContext P_BeginScope(P_Parser* parser, P_ScopeType scope_type) {
    P_ScopeContext context;
    
    // Temporary until i figure out a way to handle closures
    if (scope_type == ScopeType_Lambda) {
        context.prev_was_in_private_scope = parser->is_in_private_scope;
        parser->is_in_private_scope = true;
    }
    
    // Push on the scope type stack
    parser->scopetype_stack[parser->scopetype_tos++] = scope_type;
    
    context.prev_function_body_ret = parser->function_body_ret;
    context.prev_directly_in_func_body = parser->is_directly_in_func_body;
    parser->scope_depth++;
    
    return context;
}

static void P_EndScope(P_Parser* parser, P_ScopeContext context) {
    parser->function_body_ret = context.prev_function_body_ret;
    parser->is_directly_in_func_body = context.prev_directly_in_func_body;
    parser->is_in_private_scope = context.prev_was_in_private_scope;
    
    // Pop from the scope type stack
    parser->scopetype_tos--;
    
    // Remove all variables from the current scope
    for (u32 i = 0; i < variables.capacity; i++) {
        if (variables.entries[i].key.depth == parser->scope_depth)
            var_hash_table_del(&variables, variables.entries[i].key);
    }
    
    // Remove all functions from the current scope
    for (u32 i = 0; i < functions.capacity; i++) {
        if (functions.entries[i].key.depth == parser->scope_depth)
            func_hash_table_del(&functions, functions.entries[i].key);
    }
    
    parser->scope_depth--;
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
    P_Precedence prefix_precedence;
    P_Precedence infix_precedence;
} P_ParseRule;

//~ AST Nodes
static P_Expr* P_MakeIntNode(P_Parser* parser, i32 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_IntLit;
    expr->ret_type = ValueType_Integer;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.integer_lit = value;
    return expr;
}

static P_Expr* P_MakeLongNode(P_Parser* parser, i64 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_LongLit;
    expr->ret_type = ValueType_Long;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.long_lit = value;
    return expr;
}

static P_Expr* P_MakeFloatNode(P_Parser* parser, f32 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_FloatLit;
    expr->ret_type = ValueType_Float;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.float_lit = value;
    return expr;
}

static P_Expr* P_MakeDoubleNode(P_Parser* parser, f64 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_DoubleLit;
    expr->ret_type = ValueType_Double;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.double_lit = value;
    return expr;
}

static P_Expr* P_MakeStringNode(P_Parser* parser, string value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_StringLit;
    expr->ret_type = ValueType_String;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.string_lit = value;
    return expr;
}

static P_Expr* P_MakeCharNode(P_Parser* parser, string value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_CharLit;
    expr->ret_type = ValueType_Char;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.char_lit = value;
    return expr;
}

static P_Expr* P_MakeBoolNode(P_Parser* parser, b8 value) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_BoolLit;
    expr->ret_type = ValueType_Bool;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.bool_lit = value;
    return expr;
}

static P_Expr* P_MakeNullptrNode(P_Parser* parser) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Nullptr;
    expr->ret_type = ValueType_VoidPointer;
    expr->is_constant = true;
    expr->can_assign = false;
    return expr;
}

static P_Expr* P_MakeArrayLiteralNode(P_Parser* parser, P_ValueType type, expr_array array) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_ArrayLit;
    expr->ret_type = type;
    expr->can_assign = false;
    expr->is_constant = false; // Maybe change this
    expr->op.array = array;
    return expr;
}

static P_Expr* P_MakeUnaryNode(P_Parser* parser, L_TokenType type, P_Expr* operand, P_ValueType ret_type) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Unary;
    expr->ret_type = ret_type;
    expr->can_assign = false;
    expr->is_constant = operand->is_constant;
    expr->op.unary.operator = type;
    expr->op.unary.operand = operand;
    return expr;
}

static P_Expr* P_MakeBinaryNode(P_Parser* parser, L_TokenType type, P_Expr* left, P_Expr* right, P_ValueType ret_type) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Binary;
    expr->ret_type = ret_type;
    expr->can_assign = false;
    expr->is_constant = left->is_constant && right->is_constant;
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
    expr->is_constant = value->is_constant;
    expr->op.assignment.name = name;
    expr->op.assignment.value = value;
    return expr;
}

static P_Expr* P_MakeCastNode(P_Parser* parser, P_ValueType type, P_Expr* to_be_casted) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Cast;
    expr->ret_type = type;
    expr->is_constant = to_be_casted->is_constant;
    expr->can_assign = false;
    expr->op.cast = to_be_casted;
    return expr;
}

static P_Expr* P_MakeIndexNode(P_Parser* parser, P_ValueType type, P_Expr* left, P_Expr* e) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Index;
    expr->ret_type = type;
    expr->can_assign = true;
    expr->is_constant = false;
    expr->op.index.operand = left;
    expr->op.index.index = e;
    return expr;
}

static P_Expr* P_MakeAddrNode(P_Parser* parser, P_ValueType type, P_Expr* e) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Addr;
    expr->ret_type = type;
    expr->can_assign = false;
    expr->is_constant = false;
    expr->op.addr = e;
    return expr;
}

static P_Expr* P_MakeDerefNode(P_Parser* parser, P_ValueType type, P_Expr* e) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Deref;
    expr->ret_type = type;
    expr->can_assign = true;
    expr->is_constant = false;
    expr->op.deref = e;
    return expr;
}

static P_Expr* P_MakeDotNode(P_Parser* parser, P_ValueType type, P_Expr* left, string right) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Dot;
    expr->ret_type = type;
    expr->can_assign = true;
    expr->is_constant = false;
    expr->op.dot.left = left;
    expr->op.dot.right = right;
    return expr;
}

static P_Expr* P_MakeEnumDotNode(P_Parser* parser, P_ValueType type, string left, string right) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_EnumDot;
    expr->ret_type = type;
    expr->can_assign = false;
    expr->is_constant = false;
    expr->op.enum_dot.left = left;
    expr->op.enum_dot.right = right;
    return expr;
}

static P_Expr* P_MakeVariableNode(P_Parser* parser, string name, P_ValueType type) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Variable;
    expr->ret_type = type;
    expr->can_assign = true;
    expr->is_constant = false;
    expr->op.variable = name;
    return expr;
}

static P_Expr* P_MakeTypenameNode(P_Parser* parser, P_ValueType name) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Typename;
    expr->ret_type = ValueType_Invalid;
    expr->can_assign = true;
    expr->is_constant = true;
    expr->op.typename = name;
    return expr;
}

static P_Expr* P_MakeFuncnameNode(P_Parser* parser, P_ValueType type, string name) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Funcname;
    expr->ret_type = type;
    expr->can_assign = false;
    expr->is_constant = true;
    expr->op.funcname = name;
    return expr;
}

static P_Expr* P_MakeLambdaNode(P_Parser* parser, P_ValueType type, string name) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Lambda;
    expr->ret_type = type;
    expr->can_assign = false;
    expr->is_constant = false;
    expr->op.funcname = name;
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

static P_Expr* P_MakeCallNode(P_Parser* parser, P_Expr* left, P_ValueType type, P_Expr** exprs, u32 call_arity) {
    P_Expr* expr = arena_alloc(&parser->arena, sizeof(P_Expr));
    expr->type = ExprType_Call;
    expr->ret_type = type;
    expr->can_assign = false;
    expr->is_constant = false;
    expr->op.call.left = left;
    expr->op.call.params = exprs;
    expr->op.call.call_arity = call_arity;
    return expr;
}

static P_Stmt* P_MakeNothingNode(P_Parser* parser) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Nothing;
    stmt->next = nullptr;
    return stmt;
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

static P_Stmt* P_MakeForStmtNode(P_Parser* parser, P_Stmt* init, P_Expr* condn, P_Expr* loopexec, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_For;
    stmt->next = nullptr;
    stmt->op.for_s.init = init;
    stmt->op.for_s.condition = condn;
    stmt->op.for_s.loopexec = loopexec;
    stmt->op.for_s.then = then;
    return stmt;
}

static P_Stmt* P_MakeSwitchStmtNode(P_Parser* parser, P_Expr* switched, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Switch;
    stmt->next = nullptr;
    stmt->op.switch_s.switched = switched;
    stmt->op.switch_s.then = then;
    return stmt;
}

static P_Stmt* P_MakeMatchStmtNode(P_Parser* parser, P_Expr* matched, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Match;
    stmt->next = nullptr;
    stmt->op.match_s.matched = matched;
    stmt->op.match_s.then = then;
    return stmt;
}

static P_Stmt* P_MakeCaseStmtNode(P_Parser* parser, P_Expr* value, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Case;
    stmt->next = nullptr;
    stmt->op.case_s.value = value;
    stmt->op.case_s.then = then;
    return stmt;
}

static P_Stmt* P_MakeMatchedCaseStmtNode(P_Parser* parser, P_Expr* value, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_MatchCase;
    stmt->next = nullptr;
    stmt->op.mcase_s.value = value;
    stmt->op.mcase_s.then = then;
    return stmt;
}

static P_Stmt* P_MakeDefaultStmtNode(P_Parser* parser, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Default;
    stmt->next = nullptr;
    stmt->op.default_s.then = then;
    return stmt;
}

static P_Stmt* P_MakeMatchedDefaultStmtNode(P_Parser* parser, P_Stmt* then) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_MatchDefault;
    stmt->next = nullptr;
    stmt->op.mdefault_s.then = then;
    return stmt;
}

static P_Stmt* P_MakeBreakStmtNode(P_Parser* parser) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Break;
    stmt->next = nullptr;
    return stmt;
}

static P_Stmt* P_MakeContinueStmtNode(P_Parser* parser) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_Continue;
    stmt->next = nullptr;
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

static P_Stmt* P_MakeVarDeclAssignStmtNode(P_Parser* parser, P_ValueType type, string name, P_Expr* val) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_VarDeclAssign;
    stmt->next = nullptr;
    stmt->op.var_decl_assign.type = type;
    stmt->op.var_decl_assign.name = name;
    stmt->op.var_decl_assign.val = val;
    return stmt;
}

static P_Stmt* P_MakeEnumDeclStmtNode(P_Parser* parser, string name, u32 count, string_list names) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_EnumDecl;
    stmt->next = nullptr;
    stmt->op.enum_decl.name = name;
    stmt->op.enum_decl.member_count = count;
    stmt->op.enum_decl.member_names = names;
    return stmt;
}

static P_Stmt* P_MakeStructDeclStmtNode(P_Parser* parser, string name, u32 count, value_type_list types, string_list names) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_StructDecl;
    stmt->next = nullptr;
    stmt->op.struct_decl.name = name;
    stmt->op.struct_decl.member_count = count;
    stmt->op.struct_decl.member_types = types;
    stmt->op.struct_decl.member_names = names;
    return stmt;
}

static P_Stmt* P_MakeFuncStmtNode(P_Parser* parser, P_ValueType type, string name, u32 arity, value_type_list param_types, string_list param_names, b8 varargs) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_FuncDecl;
    stmt->next = nullptr;
    stmt->op.func_decl.type = type;
    stmt->op.func_decl.name = name;
    stmt->op.func_decl.arity = arity;
    stmt->op.func_decl.param_types = param_types;
    stmt->op.func_decl.param_names = param_names;
    stmt->op.func_decl.block = nullptr;
    stmt->op.func_decl.varargs = varargs;
    return stmt;
}

static P_Stmt* P_MakeNativeFuncStmtNode(P_Parser* parser, P_ValueType type, string name, u32 arity, value_type_list param_types, string_list param_names) {
    P_Stmt* stmt = arena_alloc(&parser->arena, sizeof(P_Stmt));
    stmt->type = StmtType_NativeFuncDecl;
    stmt->next = nullptr;
    stmt->op.native_func_decl = name;
    return stmt;
}

static P_PreStmt* P_MakePreFuncStmtNode(P_Parser* parser, P_ValueType type, string name, u32 arity, value_type_list param_types, string_list param_names) {
    P_PreStmt* stmt = arena_alloc(&parser->arena, sizeof(P_PreStmt));
    stmt->type = PreStmtType_ForwardDecl;
    stmt->next = nullptr;
    stmt->op.forward_decl.type = type;
    stmt->op.forward_decl.name = name;
    stmt->op.forward_decl.arity = arity;
    stmt->op.forward_decl.param_types = param_types;
    stmt->op.forward_decl.param_names = param_names;
    return stmt;
}

//~ Expressions
static P_Expr* P_ExprPrecedence(P_Parser* parser, P_Precedence precedence);
static P_ParseRule* P_GetRule(L_TokenType type);
static P_Expr* P_Expression(P_Parser* parser);
static P_Stmt* P_Statement(P_Parser* parser);
static P_Stmt* P_Declaration(P_Parser* parser);

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

static P_Expr* P_ExprNullptr(P_Parser* parser) {
    return P_MakeNullptrNode(parser);
}

static P_Expr* P_ExprArray(P_Parser* parser) {
    if (P_Match(parser, TokenType_CloseBrace))
        return P_MakeArrayLiteralNode(parser, ValueType_Any, (expr_array) {0});
    
    expr_array arr = {0};
    P_Expr* curr = P_Expression(parser);
    P_ValueType array_type = curr->ret_type;
    while (curr != nullptr) {
        expr_array_add(&parser->arena, &arr, curr);
        if (P_Match(parser, TokenType_CloseBrace)) break;
        P_Consume(parser, TokenType_Comma, str_lit("Expected , or }\n"));
        curr = P_Expression(parser);
        if (!type_check(curr->ret_type, array_type))
            report_error(parser, str_lit("Arrays cannot contain expressions of multiple types.\n"));
    }
    array_type = P_PushArrayType(parser, array_type, arr.count);
    return P_MakeArrayLiteralNode(parser, array_type, arr);
}

static P_Expr* P_ExprVar(P_Parser* parser) {
    string name = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
    
    // If I find a (, its a function call
    if (P_Match(parser, TokenType_OpenParenthesis)) {
        P_Expr* params[256] = {0};
        value_type_list param_types = {0};
        u32 call_arity = 0;
        while (!P_Match(parser, TokenType_CloseParenthesis)) {
            if (call_arity != 0) {
                if (!P_Match(parser, TokenType_Comma)) {
                    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) or , after parameter\n"));
                    break;
                }
            }
            
            params[call_arity] = P_Expression(parser);
            if (params[call_arity] == nullptr) return nullptr;
            value_type_list_push(&parser->arena, &param_types, params[call_arity]->ret_type);
            call_arity++;
        }
        
        // FIXME(voxel): memcpy wack. should be an arena method
        P_Expr** param_buffer = arena_alloc(&parser->arena, call_arity * sizeof(P_Expr*));
        memcpy(param_buffer, params, call_arity * sizeof(P_Expr*));
        
        func_entry_key key = { .name = name, .depth = parser->scope_depth };
        func_entry_val* value = nullptr;
        
        u32 subset_match = 1024; // Invalid cuz max param count is 256
        while (key.depth != -1) {
            if (func_hash_table_get(&functions, key, &param_types, &value, &subset_match, true)) {
                // Convert any ref takers to a &(Addr) node
                value_type_list_node* curr = value->param_types.first;
                u32 i = 0;
                while (curr != nullptr) {
                    if (is_ref(&curr->type)) {
                        P_Expr* e = param_buffer[i];
                        P_Expr* a = P_MakeAddrNode(parser, curr->type, e);
                        param_buffer[i] = a;
                    }
                    curr = curr->next;
                    i++;
                }
                
                return P_MakeFuncCallNode(parser, value->is_native ? name : value->mangled_name, value->value, param_buffer, call_arity);
            }
            key.depth--;
        }
        // Reset key depth for a fuzzy type search
        key.depth = parser->scope_depth;
        while (key.depth != -1) {
            if (func_hash_table_get(&functions, key, &param_types, &value, &subset_match, false)) {
                // Convert any ref takers to a &(Addr) node
                value_type_list_node* curr = value->param_types.first;
                u32 i = 0;
                while (curr != nullptr) {
                    if (is_ref(&curr->type)) {
                        P_Expr* e = param_buffer[i];
                        if (!e->can_assign)
                            report_error(parser, str_lit("Cannot pass non assignable value to reference parameter\n"));
                        P_Expr* a = P_MakeAddrNode(parser, curr->type, e);
                        param_buffer[i] = a;
                    }
                    curr = curr->next;
                    i++;
                }
                
                return P_MakeFuncCallNode(parser, value->is_native ? name : value->mangled_name, value->value, param_buffer, call_arity);
            }
            key.depth--;
        }
        
        P_ValueType ret;
        if (P_GetVariable(parser, name, &ret)) {
            if (ret.type == ValueTypeType_FuncPointer) {
                // Type chack arguments
                value_type_list_node* curr_test = ret.op.func_ptr.func_param_types->first;
                value_type_list_node* curr = param_types.first;
                u32 i = 0;
                while (!(curr_test == nullptr || curr == nullptr)) {
                    
                    if (!str_eq(str_lit("..."), curr_test->type.full_type)) {
                        if (!type_check(curr->type, curr_test->type))
                            report_error(parser, str_lit("Invalid arguments to function call\n"));
                        
                        // Convert any ref takers to a &(Addr) node
                        if (is_ref(&curr_test->type)) {
                            P_Expr* e = param_buffer[i];
                            if (!e->can_assign)
                                report_error(parser, str_lit("Cannot pass non assignable value to reference parameter\n"));
                            P_Expr* a = P_MakeAddrNode(parser, curr->type, e);
                            param_buffer[i] = a;
                        }
                        
                        curr_test = curr_test->next;
                        curr = curr->next;
                    } else {
                        if (curr == param_types.last) {
                            curr = curr->next;
                            curr_test = curr_test->next;
                        } else curr = curr->next;
                    }
                    
                    i++;
                }
                
                if (curr_test == nullptr && curr == nullptr) {
                    return P_MakeFuncCallNode(parser, name, *ret.op.func_ptr.ret_type, param_buffer, call_arity);
                }
                report_error(parser, str_lit("Invalid arguments to function call\n"));
            } else {
                report_error(parser, str_lit("Cannot call variable that isn't a function pointer\n"));
            }
        }
        
        report_error(parser, str_lit("Undefined function %.*s with provided parameters\n"), str_expand(name));
        
    } else {
        
        P_ValueType value = ValueType_Invalid;
        if (P_GetVariable(parser, name, &value)) {
            if (is_ref(&value)) {
                P_ValueType ret = P_ReduceType(value);
                return P_MakeDerefNode(parser, ret, P_MakeVariableNode(parser, name, value));
            }
            return P_MakeVariableNode(parser, name, value);
        }
        
        P_Container* type = type_array_get(parser, name, parser->scope_depth);
        if (type != nullptr) {
            type_mod_array mods = {0};
            P_ConsumeTypeMods(parser, &mods);
            u64 complete_length = ((u64)parser->previous.start + (u64)parser->previous.length) - (u64)name.str;
            return P_MakeTypenameNode(parser, (P_ValueType) { .base_type = name, .full_type = (string) { .str = name.str, .size = complete_length }, .mods = mods.elements, .mod_ct = mods.count });
        }
        
        func_entry_key fnkey = { .name = name, .depth = parser->scope_depth };
        while (fnkey.depth != -1) {
            if (func_hash_table_has_name(&functions, fnkey)) {
                // Return a stub name
                return P_MakeFuncnameNode(parser, (P_ValueType){ .type = ValueTypeType_FuncPointer }, name);
            }
            fnkey.depth--;
        }
        
        report_error(parser, str_lit("Undefined variable %.*s\n"), str_expand(name));
    }
    return nullptr;
}

static P_Expr* P_ExprLambda(P_Parser* parser) {
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected (\n"));
    
    // Arena allocation bcoz it's reused for creating the type
    P_ScopeContext scope_context = P_BeginScope(parser, ScopeType_Lambda);
    
    value_type_list* params = arena_alloc(&parser->arena, sizeof(value_type_list));
    string_list param_names = {0};
    u32 arity = 0;
    b8 varargs = false;
    
    parser->block_stmt_should_begin_scope = false;
    
    parser->all_code_paths_return = false;
    parser->encountered_return = false;
    parser->function_body_ret = ValueType_Any;
    parser->is_directly_in_func_body = true;
    
    while (!P_Match(parser, TokenType_CloseParenthesis)) {
        if (P_Match(parser, TokenType_Dot)) {
            P_Consume(parser, TokenType_Dot, str_lit("Expected .. after .\n"));
            P_Consume(parser, TokenType_Dot, str_lit("Expected .. after .\n"));
            
            if (arity == 0)
                report_error(parser, str_lit("Only varargs function not allowed\n"));
            
            value_type_list_push(&parser->arena, params, value_type_abs("..."));
            
            P_Consume(parser, TokenType_Identifier, str_lit("Expected param name\n"));
            varargs = true;
            string varargs_name = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
            string_list_push(&parser->arena, &param_names, varargs_name);
            
            P_Consume(parser, TokenType_CloseParenthesis, str_lit("Varargs can only be the last argument for a function\n"));
            arity++;
            break;
        }
        
        P_ValueType param_type = P_ConsumeType(parser, str_lit("Expected type\n"));
        value_type_list_push(&parser->arena, params, param_type);
        if (is_array(&param_type))
            report_error(parser, str_lit("Cannot Pass Array type as parameter\n"));
        
        P_Consume(parser, TokenType_Identifier, str_lit("Expected param name\n"));
        string param_name = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
        string_list_push(&parser->arena, &param_names, param_name);
        
        var_entry_key key = (var_entry_key) { .name = param_name, .depth = parser->scope_depth };
        P_ValueType test;
        if (!var_hash_table_get(&variables, key, &test))
            var_hash_table_set(&variables, key, param_type);
        else
            report_error(parser, str_lit("Multiple Parameters with the same name\n"));
        
        arity++;
        if (!P_Match(parser, TokenType_Comma)) {
            P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after params to lambda\n"));
            break;
        }
    }
    
    P_Consume(parser, TokenType_Arrow, str_lit("Expected => after ) in lambda\n"));
    
    string additional = {0};
    if (varargs) additional = str_cat(&parser->arena, additional, str_lit("varargs"));
    string mangled = P_FuncNameMangle(parser, str_from_format(&parser->arena, "lambda%d", parser->lambda_number++), varargs ? arity - 1 : arity, *params, additional);
    
    P_Stmt* lambda = P_Declaration(parser);
    
    P_ValueType* return_type;
    if (parser->encountered_return) {
        return_type = arena_alloc(&parser->arena, sizeof(P_ValueType));
        memcpy(return_type, &parser->function_body_ret, sizeof(P_ValueType));
    } else {
        return_type = &ValueType_Void;
        parser->all_code_paths_return = true;
    }
    
    P_Stmt* func = P_MakeFuncStmtNode(parser, *return_type, mangled, arity, *params, param_names, varargs);
    func->op.func_decl.block = lambda;
    
    // Push onto lambda SLL
    if (parser->lambda_functions_curr != nullptr) {
        parser->lambda_functions_curr->next = func;
        parser->lambda_functions_curr = func;
    } else {
        parser->lambda_functions_start = func;
        parser->lambda_functions_curr = func;
    }
    if (!parser->all_code_paths_return) report_error(parser, str_lit("Not all code paths of lambda return a value\n"));
    
    parser->block_stmt_should_begin_scope = true;
    P_EndScope(parser, scope_context);
    
    return P_MakeLambdaNode(parser, P_CreateFnpointerType(parser, return_type, params), mangled);
}

static P_Expr* P_ExprCall(P_Parser* parser, P_Expr* left) {
    if (left->ret_type.type != ValueTypeType_FuncPointer) {
        report_error(parser, str_lit("Cannot call a non function pointer type\n"));
        return nullptr;
    }
    
    P_Expr* params[256] = {0};
    value_type_list param_types = {0};
    u32 call_arity = 0;
    while (!P_Match(parser, TokenType_CloseParenthesis)) {
        if (call_arity != 0) {
            if (!P_Match(parser, TokenType_Comma)) {
                P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) or , after parameter\n"));
                break;
            }
        }
        
        params[call_arity] = P_Expression(parser);
        if (params[call_arity] == nullptr) return nullptr;
        value_type_list_push(&parser->arena, &param_types, params[call_arity]->ret_type);
        call_arity++;
    }
    
    // FIXME(voxel): memcpy wack. should be an arena method
    P_Expr** param_buffer = arena_alloc(&parser->arena, call_arity * sizeof(P_Expr*));
    memcpy(param_buffer, params, call_arity * sizeof(P_Expr*));
    
    if (left->type == ExprType_Funcname) {
        // Fix funcname if possible.
        // Only possible if there is only one overload for the funcname.... IS IT?
        func_entry_val* val = nullptr;
        u32 subset;
        func_entry_key key = { .name = left->op.funcname, .depth = parser->scope_depth };
        
        while (key.depth != -1) {
            if (func_hash_table_get(&functions, key, &param_types, &val,  &subset, false)) {
                left->ret_type = P_CreateFnpointerType(parser, &val->value, &val->param_types);
                left->op.funcname = val->mangled_name;
            }
            key.depth--;
        }
    }
    
    // Check required params against what we got
    // If it wants more parameters than currently provided, just dont check anything. parameters are wrong
    if (left->ret_type.op.func_ptr.func_param_types->node_count - 1 <= param_types.node_count) {
        
        // Check string_list equals. (can be a subset, so not using string_list_equals)
        value_type_list_node* curr_test = left->ret_type.op.func_ptr.func_param_types->first;
        value_type_list_node* curr = param_types.first;
        while (!(curr_test == nullptr || curr == nullptr)) {
            if (!str_eq(str_lit("..."), curr_test->type.full_type)) {
                if (!type_check(curr->type, curr_test->type)) break;
                curr_test = curr_test->next;
                curr = curr->next;
            } else {
                curr = curr->next;
            }
        }
        
        // If the while loop exited normally, we found a type match
        if (!(curr_test == nullptr || curr == nullptr)) {
            report_error(parser, str_lit("Wrong Parameters passed to function\n"));
            return nullptr;
        }
    } else {
        report_error(parser, str_lit("Wrong Parameters passed to function\n"));
        return nullptr;
    }
    
    value_type_list_node* curr = param_types.first;
    u32 i = 0;
    while (curr != nullptr) {
        if (is_ref(&curr->type)) {
            P_Expr* e = param_buffer[i];
            if (!e->can_assign)
                report_error(parser, str_lit("Cannot pass non assignable value to reference parameter\n"));
            P_Expr* a = P_MakeAddrNode(parser, curr->type, e);
            param_buffer[i] = a;
        }
        curr = curr->next;
        i++;
    }
    
    // Checks passed, Create the call node.
    return P_MakeCallNode(parser, left, *left->ret_type.op.func_ptr.ret_type, param_buffer, call_arity);
}

static P_Expr* P_ExprPrimitiveTypename(P_Parser* parser) {
    string base_type = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
    
    type_mod_array mods = {0};
    P_ConsumeTypeMods(parser, &mods);
    
    u64 complete_length = ((u64)parser->previous.start + (u64)parser->previous.length) - (u64)base_type.str;
    
    return P_MakeTypenameNode(parser, (P_ValueType) { .base_type = base_type, .full_type = (string) { .str = base_type.str, .size = complete_length }, .mods = mods.elements, .mod_ct = mods.count });
}

static P_Expr* P_ExprAssign(P_Parser* parser, P_Expr* left) {
    if (left != nullptr) {
        if (!left->can_assign)
            report_error(parser, str_lit("Required variable name before = sign\n"));
        
        P_Expr* xpr = P_Expression(parser);
        if (xpr == nullptr) return nullptr;
        
        if (is_ref(&left->ret_type)) {
            P_ValueType m = P_ReduceType(left->ret_type);
            if (type_check(xpr->ret_type, m)) {
                return P_MakeAssignmentNode(parser, left, P_MakeAddrNode(parser, m, xpr));
            }
        }
        
        if (left->ret_type.type == ValueTypeType_FuncPointer) {
            if (xpr->type == ExprType_Funcname) {
                string start_name = xpr->op.funcname;
                // Differentiate the Funcname based on what params we need
                func_entry_key key = { .name = xpr->op.funcname, .depth = parser->scope_depth };
                func_entry_val* val = nullptr;
                u32 subset;
                while (key.depth != -1) {
                    if (func_hash_table_get(&functions, key, left->ret_type.op.func_ptr.func_param_types, &val, &subset, true)) {
                        // Fix the xpr
                        xpr->ret_type.op.func_ptr.func_param_types = &val->param_types;
                        xpr->ret_type.op.func_ptr.ret_type = left->ret_type.op.func_ptr.ret_type;
                        xpr->ret_type.full_type = left->ret_type.full_type;
                        xpr->op.funcname = val->mangled_name;
                        
                        return P_MakeAssignmentNode(parser, left, xpr);
                    }
                    key.depth--;
                }
                report_error(parser, str_lit("No overload for function %.*s takes the appropriate parameters\n"), str_expand(start_name));
            } else if (xpr->ret_type.type == ValueTypeType_FuncPointer) {
                
                if (!type_check(xpr->ret_type, left->ret_type)) {
                    report_error(parser, str_lit("Incompatible function pointer types\n"));
                } else {
                    return P_MakeAssignmentNode(parser, left, xpr);
                }
            } else {
                report_error(parser, str_lit("Expected function pointer\n"));
            }
        }
        
        if (type_check(xpr->ret_type, left->ret_type)) {
            return P_MakeAssignmentNode(parser, left, xpr);
        }
        
        report_error(parser, str_lit("Cannot assign %.*s to variable\n"), str_expand(xpr->ret_type.full_type));
    }
    return nullptr;
}

static P_Expr* P_ExprGroup(P_Parser* parser) {
    P_Expr* in = P_Expression(parser);
    if (in == nullptr) return nullptr;
    
    if (in->type == ExprType_Typename) {
        // Explicit Cast syntax
        P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after typename\n"));
        P_Expr* to_be_casted = P_Expression(parser);
        
        b8 allowed_cast = false;
        if (!type_check(to_be_casted->ret_type, in->op.typename)) {
            
            if (is_ptr(&in->op.typename)) {
                if (is_ptr(&to_be_casted->ret_type))
                    allowed_cast = true;
            }
        } else allowed_cast = true;
        
        if (allowed_cast)
            return P_MakeCastNode(parser, in->op.typename, to_be_casted);
        
        report_error(parser, str_lit("Cannot cast from %.*s to %.*s\n"), str_expand(to_be_casted->ret_type.full_type), str_expand(in->op.typename.full_type));
        return nullptr;
    }
    
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression\n"));
    return in;
}

static P_Expr* P_ExprIndex(P_Parser* parser, P_Expr* left) {
    if (!(is_ptr(&left->ret_type) || is_array(&left->ret_type)))
        report_error(parser, str_lit("Cannot Apply [] operator to expression of type %.*s\n"), left->ret_type.full_type);
    
    P_Expr* e = P_Expression(parser);
    if (!type_check(e->ret_type, ValueType_Integer))
        report_error(parser, str_lit("The [] Operator expects an Integer. Got %.*s\n"), str_expand(e->ret_type.full_type));
    
    P_Consume(parser, TokenType_CloseBracket, str_lit("Unclosed [\n"));
    P_ValueType ret = P_ReduceType(left->ret_type);
    return P_MakeIndexNode(parser, ret, left, e);
}

static P_Expr* P_ExprAddr(P_Parser* parser) {
    P_Expr* e = P_Expression(parser);
    if (!e->can_assign)
        report_error(parser, str_lit("Cannot Get Address of Non assignable expression\n"));
    P_ValueType ret = P_PushPointerType(parser, e->ret_type);
    return P_MakeAddrNode(parser, ret, e);
}

static P_Expr* P_ExprDeref(P_Parser* parser) {
    P_Expr* e = P_Expression(parser);
    
    if (!is_ptr(&e->ret_type))
        report_error(parser, str_lit("Cannot Dereference expression of type %.*s\n"), str_expand(e->ret_type.full_type));
    
    P_ValueType ret = P_ReduceType(e->ret_type);
    return P_MakeDerefNode(parser, ret, e);
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
    P_Expr* right = P_ExprPrecedence(parser, (P_Precedence)(rule->infix_precedence + 1));
    P_ValueType ret_type;
    switch (op_type) {
        case TokenType_Plus: {
            P_BindDouble(parser, left, right, pairs_operator_arithmetic_term, sizeof(pairs_operator_arithmetic_term), str_lit("Cannot apply binary operator + to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Minus: {
            P_BindDouble(parser, left, right, pairs_operator_arithmetic_term, sizeof(pairs_operator_arithmetic_term), str_lit("Cannot apply binary operator - to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Star: {
            P_BindDouble(parser, left, right, pairs_operator_arithmetic_factor, sizeof(pairs_operator_arithmetic_factor), str_lit("Cannot apply binary operator * to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Slash: {
            P_BindDouble(parser, left, right, pairs_operator_arithmetic_factor, sizeof(pairs_operator_arithmetic_factor), str_lit("Cannot apply binary operator / to %s and %s\n"));
            ret_type = P_GetNumberBinaryValType(left, right);
        } break;
        
        case TokenType_Percent: {
            P_BindDouble(parser, left, right, pairs_operator_arithmetic_factor, sizeof(pairs_operator_arithmetic_factor), str_lit("Cannot apply binary operator % to %s and %s\n"));
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
    if (left != nullptr) {
        if (left->type == ExprType_Typename) {
            if (!container_type_exists(parser, left->op.typename.full_type, parser->scope_depth)) {
                report_error(parser, str_lit("%.*s Is not a namespace or enum.\n"));
                return nullptr;
            }
            
            P_Consume(parser, TokenType_Identifier, str_lit("Expected Member name after .\n"));
            string reqd = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
            P_Container* type = type_array_get(parser, left->op.typename.full_type, parser->scope_depth);
            
            if (!member_exists(type, reqd)) {
                report_error(parser, str_lit("No member %.*s in enum %.*s\n"), str_expand(reqd), str_expand(left->op.typename.full_type));
            }
            
            // NOTE(voxel): This is always ValueType_Integer for now.
            // NOTE(voxel): But if I wanna support enums of different types, this will be different
            P_ValueType member_type = member_type_get(type, reqd);
            return P_MakeEnumDotNode(parser, member_type, left->op.typename.full_type, reqd);
            
        } else {
            P_ValueType valtype = left->ret_type;
            
            if (!container_type_exists(parser, valtype.full_type, parser->scope_depth)) {
                report_error(parser, str_lit("Cannot apply . operator\n"));
                return nullptr;
            }
            
            P_Consume(parser, TokenType_Identifier, str_lit("Expected Member name after .\n"));
            string reqd = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
            P_Container* type = type_array_get(parser, valtype.full_type, parser->scope_depth);
            if (!member_exists(type, reqd)) {
                report_error(parser, str_lit("No member %.*s in struct %.*s\n"), str_expand(reqd), str_expand(valtype.full_type));
            }
            P_ValueType member_type = member_type_get(type, reqd);
            return P_MakeDotNode(parser, member_type, left, reqd);
        }
    }
    
    report_error(parser, str_lit("Cannot apply . operator\n"));
    return nullptr;
}

P_ParseRule parse_rules[] = {
    [TokenType_Error]              = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_EOF]                = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Whitespace]         = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Identifier]         = { P_ExprVar,     nullptr, Prec_Primary, Prec_None },
    [TokenType_IntLit]             = { P_ExprInteger, nullptr, Prec_Primary, Prec_None },
    [TokenType_FloatLit]           = { P_ExprFloat,   nullptr, Prec_Primary, Prec_None },
    [TokenType_DoubleLit]          = { P_ExprDouble,  nullptr, Prec_Primary, Prec_None },
    [TokenType_LongLit]            = { P_ExprLong,    nullptr, Prec_Primary, Prec_None },
    [TokenType_StringLit]          = { P_ExprString,  nullptr, Prec_Primary, Prec_None },
    [TokenType_CharLit]            = { P_ExprChar,    nullptr, Prec_Primary, Prec_None },
    [TokenType_Plus]               = { P_ExprUnary, P_ExprBinary, Prec_Unary, Prec_Term   },
    [TokenType_Minus]              = { P_ExprUnary, P_ExprBinary, Prec_Unary, Prec_Term   },
    [TokenType_Star]               = { P_ExprDeref, P_ExprBinary, Prec_Unary, Prec_Factor },
    [TokenType_Slash]              = { nullptr,     P_ExprBinary, Prec_None,  Prec_Factor },
    [TokenType_Percent]            = { nullptr,     P_ExprBinary, Prec_None,  Prec_Factor },
    [TokenType_PlusPlus]           = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_MinusMinus]         = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Backslash]          = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Hat]                = { P_ExprLambda, P_ExprBinary, Prec_Primary, Prec_BitXor },
    [TokenType_Ampersand]          = { P_ExprAddr,   P_ExprBinary, Prec_Unary,   Prec_BitAnd },
    [TokenType_Pipe]               = { nullptr,      P_ExprBinary, Prec_None,    Prec_BitOr  },
    [TokenType_Tilde]              = { P_ExprUnary, nullptr,  Prec_Unary, Prec_None },
    [TokenType_Bang]               = { P_ExprUnary, nullptr,  Prec_Unary, Prec_None },
    [TokenType_Equal]              = { nullptr, P_ExprAssign, Prec_None, Prec_Assignment },
    [TokenType_PlusEqual]          = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_MinusEqual]         = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_StarEqual]          = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_SlashEqual]         = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_PercentEqual]       = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_AmpersandEqual]     = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_PipeEqual]          = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_HatEqual]           = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_TildeEqual]         = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_EqualEqual]         = { nullptr, P_ExprBinary, Prec_None, Prec_Equality },
    [TokenType_BangEqual]          = { nullptr, P_ExprBinary, Prec_None, Prec_Equality },
    [TokenType_Less]               = { nullptr, P_ExprBinary, Prec_None, Prec_Comparison },
    [TokenType_Greater]            = { nullptr, P_ExprBinary, Prec_None, Prec_Comparison },
    [TokenType_LessEqual]          = { nullptr, P_ExprBinary, Prec_None, Prec_Comparison },
    [TokenType_GreaterEqual]       = { nullptr, P_ExprBinary, Prec_None, Prec_Comparison },
    [TokenType_AmpersandAmpersand] = { nullptr, P_ExprBinary, Prec_None, Prec_LogAnd },
    [TokenType_PipePipe]           = { nullptr, P_ExprBinary, Prec_None, Prec_LogOr },
    [TokenType_OpenBrace]          = { P_ExprArray, nullptr,    Prec_Primary, Prec_None },
    [TokenType_OpenParenthesis]    = { P_ExprGroup, P_ExprCall, Prec_None, Prec_Call },
    [TokenType_OpenBracket]        = { nullptr, P_ExprIndex,    Prec_None, Prec_Call },
    [TokenType_CloseBrace]         = { nullptr, nullptr,   Prec_None, Prec_None },
    [TokenType_CloseParenthesis]   = { nullptr, nullptr,   Prec_None, Prec_None },
    [TokenType_CloseBracket]       = { nullptr, nullptr,   Prec_None, Prec_None },
    [TokenType_Dot]                = { nullptr, P_ExprDot, Prec_None, Prec_Call },
    [TokenType_Semicolon]          = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Colon]              = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Question]           = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Return]             = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Struct]             = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Enum]               = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Null]               = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Nullptr]            = { P_ExprNullptr, nullptr, Prec_None, Prec_None },
    [TokenType_If]                 = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Else]               = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_Do]                 = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_For]                = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_While]              = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_True]               = { P_ExprBool, nullptr, Prec_None, Prec_None },
    [TokenType_False]              = { P_ExprBool, nullptr, Prec_None, Prec_None },
    [TokenType_Int]                = { P_ExprPrimitiveTypename, nullptr, Prec_None, Prec_None },
    [TokenType_Float]              = { P_ExprPrimitiveTypename, nullptr, Prec_None, Prec_None },
    [TokenType_Bool]               = { P_ExprPrimitiveTypename, nullptr, Prec_None, Prec_None },
    [TokenType_Double]             = { P_ExprPrimitiveTypename, nullptr, Prec_None, Prec_None },
    [TokenType_Char]               = { P_ExprPrimitiveTypename, nullptr, Prec_None, Prec_None },
    [TokenType_String]             = { P_ExprPrimitiveTypename, nullptr, Prec_None, Prec_None },
    [TokenType_Long]               = { P_ExprPrimitiveTypename, nullptr, Prec_None, Prec_None },
    [TokenType_Void]               = { nullptr, nullptr, Prec_None, Prec_None },
    [TokenType_TokenTypeCount]     = { nullptr, nullptr, Prec_None, Prec_None },
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
    while (precedence <= P_GetRule(parser->current.type)->infix_precedence) {
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
static P_Stmt* P_StmtFuncDecl(P_Parser* parser, P_ValueType type, string name, b8 native) {
    value_type_list params = {0};
    string_list param_names = {0};
    u32 arity = 0;
    b8 varargs = false;
    
    if (is_array(&type))
        report_error(parser, str_lit("Cannot Return Array type from function\n"));
    
    P_ScopeContext scope_context;
    if (!native) {
        // Inner scope functions are basically lambdas
        scope_context = P_BeginScope(parser, parser->scope_depth == 1 ? ScopeType_None : ScopeType_Lambda);
        parser->all_code_paths_return = type_check(type, ValueType_Void);
        parser->encountered_return = false;
        parser->function_body_ret = type;
        parser->is_directly_in_func_body = true;
    }
    
    while (!P_Match(parser, TokenType_CloseParenthesis)) {
        if (P_Match(parser, TokenType_Dot)) {
            P_Consume(parser, TokenType_Dot, str_lit("Expected .. after .\n"));
            P_Consume(parser, TokenType_Dot, str_lit("Expected . after ..\n"));
            
            if (arity == 0)
                report_error(parser, str_lit("Only varargs function not allowed\n"));
            
            value_type_list_push(&parser->arena, &params, value_type_abs("..."));
            
            P_Consume(parser, TokenType_Identifier, str_lit("Expected param name\n"));
            varargs = true;
            string varargs_name = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
            string_list_push(&parser->arena, &param_names, varargs_name);
            
            P_Consume(parser, TokenType_CloseParenthesis, str_lit("Varargs can only be the last argument for a function\n"));
            arity++;
            break;
        }
        
        P_ValueType param_type = P_ConsumeType(parser, str_lit("Expected type\n"));
        value_type_list_push(&parser->arena, &params, param_type);
        if (is_array(&param_type))
            report_error(parser, str_lit("Cannot Pass Array type as parameter\n"));
        
        P_Consume(parser, TokenType_Identifier, str_lit("Expected param name\n"));
        string param_name = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
        string_list_push(&parser->arena, &param_names, param_name);
        
        if (!native) {
            var_entry_key key = (var_entry_key) { .name = param_name, .depth = parser->scope_depth };
            P_ValueType test;
            if (!var_hash_table_get(&variables, key, &test))
                var_hash_table_set(&variables, key, param_type);
            else
                report_error(parser, str_lit("Multiple Parameters with the same name\n"));
        }
        
        arity++;
        if (!P_Match(parser, TokenType_Comma)) {
            P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after parameters\n"));
            break;
        }
    }
    
    string additional = {0};
    if (varargs) additional = str_cat(&parser->arena, additional, str_lit("varargs"));
    
    string mangled = {0};
    if (parser->scope_depth == 1) {
        mangled = native || str_eq(str_lit("main"), name)
            ? name : P_FuncNameMangle(parser, name, varargs ? arity - 1 : arity, params, additional);
    } else {
        // Function in a scope...
        string main_part = str_cat(&parser->arena, parser->current_function, name);
        mangled = native
            ? name : P_FuncNameMangle(parser, main_part, varargs ? arity - 1 : arity, params, additional);
    }
    
    // Don't try to add the outermost scope functions since they get added during preparsing
    if (parser->scope_depth != 1 || native) {
        func_entry_key key = (func_entry_key) { .name = name, .depth = native ? parser->scope_depth : parser->scope_depth - 1 };
        // -1 When not native because the scope depth is changed by this point if the function has a body
        
        u32 subset_match = 1024;
        func_entry_val* test = nullptr;
        if (!func_hash_table_get(&functions, key, &params, &test, &subset_match, true)) {
            func_entry_val* val = arena_alloc(&parser->arena, sizeof(func_entry_val)); val->value = type;
            val->is_native = native;
            val->param_types = params;
            val->mangled_name = mangled;
            
            func_hash_table_set(&functions, key, val);
        } else report_error(parser, str_lit("Cannot redeclare function %.*s\n"), str_expand(name));
    }
    
    // Block Stuff
    P_Stmt* func = nullptr;
    
    if (!native) {
        P_Consume(parser, TokenType_OpenBrace, str_lit("Expected {\n"));
        
        func = P_MakeFuncStmtNode(parser, type, mangled, arity, params, param_names, varargs);
        
        string outer_function = parser->current_function;
        parser->current_function = mangled;
        
        P_Stmt* curr = nullptr;
        while (!P_Match(parser, TokenType_CloseBrace)) {
            P_Stmt* stmt = P_Declaration(parser);
            
            if (curr == nullptr) func->op.func_decl.block = stmt;
            else curr->next = stmt;
            
            curr = stmt;
            if (parser->current.type == TokenType_EOF)
                report_error(parser, str_lit("Unterminated Function Block. Expected }\n"));
        }
        
        parser->current_function = outer_function;
        
        if (parser->scope_depth != 1) {
            // If it's an inner scope function, Push it onto the lambda stack and return a "Nothing" Node Instead
            if (parser->lambda_functions_curr != nullptr) {
                parser->lambda_functions_curr->next = func;
                parser->lambda_functions_curr = func;
            } else {
                parser->lambda_functions_start = func;
                parser->lambda_functions_curr = func;
            }
            
            func = P_MakeNothingNode(parser);
        }
        
        P_EndScope(parser, scope_context);
        if (!parser->all_code_paths_return) report_error(parser, str_lit("Not all code paths return a value\n"));
    } else {
        P_Consume(parser, TokenType_Semicolon, str_lit("Can't have function body on a native function\n"));
        func = P_MakeNativeFuncStmtNode(parser, type, name, arity, params, param_names);
    }
    
    return func;
}

static P_Stmt* P_StmtVarDecl(P_Parser* parser, P_ValueType type, string name) {
    P_ValueType test;
    var_entry_key key = (var_entry_key) { .name = name, .depth = parser->scope_depth };
    if (!var_hash_table_get(&variables, key, &test)) {
        var_hash_table_set(&variables, key, type);
    }
    else report_error(parser, str_lit("Cannot redeclare variable %.*s\n"), str_expand(name));
    if (type_check(type, ValueType_Void))
        report_error(parser, str_lit("Cannot declare variable of type: void\n"));
    
    if (P_Match(parser, TokenType_Equal)) {
        P_Expr* value = P_Expression(parser);
        if (value == nullptr)
            return nullptr;
        
        if (is_ref(&type)) {
            if (!value->can_assign)
                report_error(parser, str_lit("Cannot pass a reference to a non assignable field\n"));
            P_ValueType m = P_ReduceType(type);
            
            if (type_check(value->ret_type, m)) {
                P_Consume(parser, TokenType_Semicolon, str_lit("Expected semicolon\n"));
                return P_MakeVarDeclAssignStmtNode(parser, type, name, P_MakeAddrNode(parser, m, value));
            }
        }
        
        if (type.type == ValueTypeType_FuncPointer) {
            if (value->type == ExprType_Funcname) {
                
                string start_name = value->op.funcname;
                // Differentiate the Funcname based on what params we need
                func_entry_key key = { .name = value->op.funcname, .depth = parser->scope_depth };
                func_entry_val* val = nullptr;
                u32 subset;
                while (key.depth != -1) {
                    if (func_hash_table_get(&functions, key, type.op.func_ptr.func_param_types, &val, &subset, true)) {
                        // Fix the value
                        value->ret_type.op.func_ptr.func_param_types = &val->param_types;
                        value->ret_type.op.func_ptr.ret_type = type.op.func_ptr.ret_type;
                        value->ret_type.full_type = type.full_type;
                        value->op.funcname = val->mangled_name;
                        
                        P_Consume(parser, TokenType_Semicolon, str_lit("Expected semicolon\n"));
                        return P_MakeVarDeclAssignStmtNode(parser, type, name, value);
                    }
                    key.depth--;
                }
                report_error(parser, str_lit("No overload for function %.*s takes the appropriate parameters\n"), str_expand(start_name));
                
            } else if (value->ret_type.type == ValueTypeType_FuncPointer) {
                
                if (!type_check(value->ret_type, type)) {
                    report_error(parser, str_lit("Incompatible function pointer types\n"));
                } else {
                    P_Consume(parser, TokenType_Semicolon, str_lit("Expected semicolon\n"));
                    return P_MakeVarDeclAssignStmtNode(parser, type, name, value);
                }
            } else {
                report_error(parser, str_lit("Expected function pointer\n"));
            }
        } else {
            if (!type_check(value->ret_type, type))
                report_error(parser, str_lit("Cannot Assign Value of Type %.*s to variable of type %.*s\n"), str_expand(value->ret_type.full_type), str_expand(type.full_type));
            P_Consume(parser, TokenType_Semicolon, str_lit("Expected semicolon\n"));
            return P_MakeVarDeclAssignStmtNode(parser, type, name, value);
        }
    }
    
    if (is_ref(&type)) {
        report_error(parser, str_lit("Uninitialized Reference Type\n"));
    }
    
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected semicolon\n"));
    return P_MakeVarDeclStmtNode(parser, type, name);
}

static P_Stmt* P_StmtStructureDecl(P_Parser* parser) {
    P_Consume(parser, TokenType_Identifier, str_lit("Expected Struct name after keyword 'struct'\n"));
    string name = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
    
    if (container_type_exists(parser, name, parser->scope_depth))
        report_error(parser, str_lit("Cannot redeclare type with name %.*s\n"), str_expand(name));
    P_Consume(parser, TokenType_OpenBrace, str_lit("Expected { after Struct Name\n"));
    
    u64 idx = types.count;
    type_array_add(&types, (P_Container) { .type = ContainerType_Struct, .name = name, .depth = parser->scope_depth });
    u32 tmp_member_count = 0;
    string_list member_names = {0};
    value_type_list member_types = {0};
    
    // Parse Members
    while (!P_Match(parser, TokenType_CloseBrace)) {
        P_ValueType type = P_ConsumeType(parser, str_lit("Expected Type or }\n"));
        if (str_eq(type.full_type, name))
            report_error(parser, str_lit("Recursive Definition of structures disallowed. You should store a pointer instead\n"));
        value_type_list_push(&parser->arena, &member_types, type);
        
        P_Consume(parser, TokenType_Identifier, str_lit("Expected member name\n"));
        string_list_push(&parser->arena, &member_names, (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length });
        
        P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after name\n"));
        tmp_member_count++;
        
        if (parser->current.type == TokenType_EOF)
            report_error(parser, str_lit("Unterminated block for structure definition\n"));
    }
    
    types.elements[idx].member_count = tmp_member_count;
    types.elements[idx].member_types = member_types;
    types.elements[idx].member_names = member_names;
    
    return P_MakeStructDeclStmtNode(parser, name, tmp_member_count, member_types, member_names);
}

static P_Stmt* P_StmtEnumerationDecl(P_Parser* parser) {
    P_Consume(parser, TokenType_Identifier, str_lit("Expected Enum name after keyword 'enum'\n"));
    string name = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
    if (container_type_exists(parser, name, parser->scope_depth))
        report_error(parser, str_lit("Cannot redeclare type with name %.*s\n"), str_expand(name));
    P_Consume(parser, TokenType_OpenBrace, str_lit("Expected { after Enum Name\n"));
    
    u64 idx = types.count;
    type_array_add(&types, (P_Container) { .type = ContainerType_Enum, .name = name, .depth = parser->scope_depth });
    u32 member_count = 0;
    string_list member_names = {0};
    value_type_list member_types = {0};
    
    // Parse Members
    while (!P_Match(parser, TokenType_CloseBrace)) {
        value_type_list_push(&parser->arena, &member_types, ValueType_Integer);
        
        P_Consume(parser, TokenType_Identifier, str_lit("Expected member name\n"));
        string_list_push(&parser->arena, &member_names, (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length });
        
        member_count++;
        if (P_Match(parser, TokenType_CloseBrace)) break;
        P_Consume(parser, TokenType_Comma, str_lit("Expected comma before next member"));
        if (parser->current.type == TokenType_EOF)
            report_error(parser, str_lit("Unterminated block for enum definition\n"));
    }
    
    types.elements[idx].member_count = member_count;
    types.elements[idx].member_types = member_types;
    types.elements[idx].member_names = member_names;
    
    return P_MakeEnumDeclStmtNode(parser, name, member_count, member_names);
}

static P_Stmt* P_StmtImport(P_Parser* parser) {
    P_Consume(parser, TokenType_StringLit, str_lit("Expected Filename after 'import'\n"));
    
    // String adjusted to be only the filename without quotes
    string name = { .str = (u8*)parser->previous.start + 1, .size = parser->previous.length - 2 };
    name = str_replace_all(&parser->arena, name, str_lit("\\"), str_lit("/"));
    
    u64 last_slash = str_find_last(parser->abspath, str_lit("/"), 0);
    
    string current_folder = (string) { .str = parser->abspath.str, .size = last_slash };
    string filename = str_cat(&parser->arena, current_folder, name);
    filename = fix_filepath(&global_arena, filename);
    
    if (string_list_contains(&imports_parsing, filename))
        return P_MakeNothingNode(parser);
    
    string_list_push(&global_arena, &imports_parsing, filename);
    
    P_Parser* child_parser = parser->sub[parser->import_number++];
    
    P_Initialize(child_parser, child_parser->source, child_parser->filename, false);
    P_Parse(child_parser);
    P_Stmt* stmt_list = child_parser->root;
    P_Stmt* curr = stmt_list;
    while (curr != nullptr) {
        if (parser->end != nullptr) {
            parser->end->next = curr;
            parser->end = curr;
        } else {
            parser->root = curr;
            parser->end = curr;
        }
        curr = curr->next;
    }
    
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after Filename\n"));
    return P_MakeNothingNode(parser);
}

static P_Stmt* P_StmtBlock(P_Parser* parser) {
    P_ScopeContext scope_context;
    b8 reset = false;
    if (parser->block_stmt_should_begin_scope) {
        scope_context = P_BeginScope(parser, ScopeType_None);
        reset = true;
    }
    
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
    
    if (reset)
        P_EndScope(parser, scope_context);
    return block;
}

static P_Stmt* P_StmtReturn(P_Parser* parser) {
    parser->encountered_return = true;
    
    if (parser->is_directly_in_func_body)
        parser->all_code_paths_return = true;
    
    P_Expr* val = P_Expression(parser);
    if (val == nullptr) return nullptr;
    
    if (parser->function_body_ret.type == ValueTypeType_FuncPointer) {
        if (val->type == ExprType_Funcname) {
            string start_name = val->op.funcname;
            // Differentiate the Funcname based on what params we need
            func_entry_key key = { .name = val->op.funcname, .depth = parser->scope_depth };
            func_entry_val* ret_val = nullptr;
            u32 subset;
            while (key.depth != -1) {
                if (func_hash_table_get(&functions, key, parser->function_body_ret.op.func_ptr.func_param_types, &ret_val, &subset, true)) {
                    // Fix the val
                    val->ret_type.op.func_ptr.func_param_types = &ret_val->param_types;
                    val->ret_type.op.func_ptr.ret_type = parser->function_body_ret.op.func_ptr.ret_type;
                    val->ret_type.full_type = parser->function_body_ret.full_type;
                    val->op.funcname = ret_val->mangled_name;
                    break;
                }
                key.depth--;
            }
            if (key.depth == -1)
                report_error(parser, str_lit("No overload for function %.*s takes the appropriate parameters\n"), str_expand(start_name));
            
        } else if (val->ret_type.type == ValueTypeType_FuncPointer) {
            
            if (!type_check(val->ret_type, parser->function_body_ret)) {
                report_error(parser, str_lit("Incompatible function pointer types\n"));
            } else {
                P_Consume(parser, TokenType_Semicolon, str_lit("Expected ;\n"));
                return P_MakeReturnStmtNode(parser, val);
            }
        } else {
            report_error(parser, str_lit("Expected Function pointer\n"));
        }
    }
    
    if (str_eq(parser->function_body_ret.full_type, ValueType_Any.full_type))
        parser->function_body_ret = val->ret_type;
    else if (!type_check(val->ret_type, parser->function_body_ret))
        report_error(parser, str_lit("Function return type mismatch. Expected %.*s\n"), str_expand(parser->function_body_ret.full_type));
    
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after return statement\n"));
    return P_MakeReturnStmtNode(parser, val);
}

static P_Stmt* P_StmtIf(P_Parser* parser) {
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected ( after if\n"));
    P_Expr* condition = P_Expression(parser);
    if (!type_check(condition->ret_type, ValueType_Bool)) {
        report_error(parser, str_lit("If statement condition doesn't resolve to boolean\n"));
    }
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression"));
    
    parser->block_stmt_should_begin_scope = false;
    P_ScopeContext scope_context = P_BeginScope(parser, ScopeType_If);
    P_Stmt* then = P_Statement(parser);
    P_EndScope(parser, scope_context);
    parser->block_stmt_should_begin_scope = true;
    
    if (P_Match(parser, TokenType_Else)) {
        parser->block_stmt_should_begin_scope = false;
        P_ScopeContext scope_context = P_BeginScope(parser, ScopeType_Else);
        P_Stmt* else_s = P_Statement(parser);
        P_EndScope(parser, scope_context);
        parser->block_stmt_should_begin_scope = true;
        
        return P_MakeIfElseStmtNode(parser, condition, then, else_s);
    }
    
    return P_MakeIfStmtNode(parser, condition, then);
}

static P_Stmt* P_StmtFor(P_Parser* parser) {
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected ( after for\n"));
    
    parser->block_stmt_should_begin_scope = false;
    P_ScopeContext scope_context = P_BeginScope(parser, ScopeType_For);
    P_Stmt* initializer = nullptr;
    if (!P_Match(parser, TokenType_Semicolon)) {
        initializer = P_Declaration(parser);
    }
    
    P_Expr* condition = P_Expression(parser);
    if (!type_check(condition->ret_type, ValueType_Bool))
        report_error(parser, str_lit("Expected expression returning bool\n"));
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ;\n"));
    P_Expr* loopexec = nullptr;
    if (!P_Match(parser, TokenType_CloseParenthesis)) {
        loopexec = P_Expression(parser);
        P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression\n"));
    }
    
    P_Stmt* then = P_Statement(parser);
    P_EndScope(parser, scope_context);
    parser->block_stmt_should_begin_scope = true;
    
    return P_MakeForStmtNode(parser, initializer, condition, loopexec, then);
}

static P_Stmt* P_StmtSwitch(P_Parser* parser) {
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected ( after switch\n"));
    P_Expr* switched = P_Expression(parser);
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression\n"));
    
    P_ValueType prev_switch_type = parser->switch_type;
    parser->switch_type = switched->ret_type;
    
    parser->block_stmt_should_begin_scope = false;
    P_ScopeContext scope_context = P_BeginScope(parser, ScopeType_Switch);
    
    P_Consume(parser, TokenType_OpenBrace, str_lit("Expected { after )\n"));
    P_Stmt* then = P_StmtBlock(parser);
    
    P_EndScope(parser, scope_context);
    parser->block_stmt_should_begin_scope = true;
    
    parser->switch_type = prev_switch_type;
    
    return P_MakeSwitchStmtNode(parser, switched, then);
}

static P_Stmt* P_StmtMatch(P_Parser* parser) {
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected ( after match\n"));
    P_Expr* matched = P_Expression(parser);
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression\n"));
    
    P_ValueType prev_switch_type = parser->switch_type;
    parser->switch_type = matched->ret_type;
    
    parser->block_stmt_should_begin_scope = false;
    P_ScopeContext scope_context = P_BeginScope(parser, ScopeType_Match);
    
    P_Consume(parser, TokenType_OpenBrace, str_lit("Expected { after )\n"));
    P_Stmt* then = P_StmtBlock(parser);
    
    P_EndScope(parser, scope_context);
    parser->block_stmt_should_begin_scope = true;
    
    parser->switch_type = prev_switch_type;
    
    return P_MakeMatchStmtNode(parser, matched, then);
}

static P_Stmt* P_StmtCase(P_Parser* parser) {
    P_Expr* value = P_Expression(parser);
    if (!str_eq(value->ret_type.full_type, parser->switch_type.full_type)) {
        report_error(parser, str_lit("Case Type doesn't match with expression required for switch. Required %.*s, got %.*s\n"), str_expand(parser->switch_type.full_type), str_expand(value->ret_type.full_type));
    }
    if (!value->is_constant)
        report_error(parser, str_lit("Case Value is not a constant\n"));
    
    P_Consume(parser, TokenType_Colon, str_lit("Expected : after expression\n"));
    
    P_ScopeType type = parser->scopetype_stack[parser->scopetype_tos-1];
    P_ScopeContext scope_context;
    if (type == ScopeType_Match) {
        parser->block_stmt_should_begin_scope = false;
        scope_context = P_BeginScope(parser, ScopeType_Case);
    } else {
        parser->scopetype_stack[parser->scopetype_tos++] = ScopeType_Case;
    }
    
    P_Stmt* then = P_Declaration(parser);
    P_Stmt* end = then;
    while (!(parser->current.type == TokenType_Case ||
             parser->current.type == TokenType_Default ||
             parser->current.type == TokenType_CloseBrace)) {
        P_Stmt* tmp = P_Declaration(parser);
        end->next = tmp;
        end = tmp;
        if (parser->current.type == TokenType_EOF) {
            report_error(parser, str_lit("Unterminated Switch Or Match Block\n"));
        }
    }
    
    if (type == ScopeType_Match) {
        P_EndScope(parser, scope_context);
        parser->block_stmt_should_begin_scope = true;
    } else {
        parser->scopetype_tos--;
    }
    
    if (type == ScopeType_Switch)
        return P_MakeCaseStmtNode(parser, value, then);
    else
        return P_MakeMatchedCaseStmtNode(parser, value, then);
}

static P_Stmt* P_StmtDefault(P_Parser* parser) {
    if (parser->encountered_default)
        report_error(parser, str_lit("Multiple Default Statements\n"));
    
    P_Consume(parser, TokenType_Colon, str_lit("Expected : after expression\n"));
    
    P_ScopeType type = parser->scopetype_stack[parser->scopetype_tos-1];
    P_ScopeContext scope_context;
    if (type == ScopeType_Match) {
        parser->block_stmt_should_begin_scope = false;
        scope_context = P_BeginScope(parser, ScopeType_Default);
    } else {
        parser->scopetype_stack[parser->scopetype_tos++] = ScopeType_Default;
    }
    
    P_Stmt* then = P_Declaration(parser);
    P_Stmt* end = then;
    while (!(parser->current.type == TokenType_Case ||
             parser->current.type == TokenType_Default ||
             parser->current.type == TokenType_CloseBrace)) {
        P_Stmt* tmp = P_Declaration(parser);
        end->next = tmp;
        end = tmp;
        if (parser->current.type == TokenType_EOF) {
            report_error(parser, str_lit("Unterminated Switch Or Match Block\n"));
        }
    }
    
    if (type == ScopeType_Match) {
        P_EndScope(parser, scope_context);
        parser->block_stmt_should_begin_scope = true;
    } else {
        parser->scopetype_tos--;
    }
    
    if (type == ScopeType_Switch)
        return P_MakeDefaultStmtNode(parser, then);
    else
        return P_MakeMatchedDefaultStmtNode(parser, then);
}

static P_Stmt* P_StmtBreak(P_Parser* parser) {
    if (!(P_IsInScope(parser, ScopeType_For)    ||
          P_IsInScope(parser, ScopeType_While)  ||
          P_IsInScope(parser, ScopeType_Switch) ||
          P_IsInScope(parser, ScopeType_Match)  ||
          P_IsInScope(parser,  ScopeType_DoWhile)))
        report_error(parser, str_lit("Cannot have break statement in this block\n"));
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after break\n"));
    return P_MakeBreakStmtNode(parser);
}

static P_Stmt* P_StmtContinue(P_Parser* parser) {
    if (!(P_IsInScope(parser, ScopeType_For)   ||
          P_IsInScope(parser, ScopeType_While) ||
          P_IsInScope(parser,  ScopeType_DoWhile)))
        report_error(parser, str_lit("Cannot have continue statement in this block\n"));
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after continue\n"));
    return P_MakeContinueStmtNode(parser);
}

static P_Stmt* P_StmtWhile(P_Parser* parser) {
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected ( after while\n"));
    P_Expr* condition = P_Expression(parser);
    if (!type_check(condition->ret_type, ValueType_Bool)) {
        report_error(parser, str_lit("While loop condition doesn't resolve to boolean\n"));
    }
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression\n"));
    
    parser->block_stmt_should_begin_scope = false;
    P_ScopeContext scope_context = P_BeginScope(parser, ScopeType_None);
    
    P_Stmt* then = P_Statement(parser);
    
    P_EndScope(parser, scope_context);
    parser->block_stmt_should_begin_scope = true;
    
    return P_MakeWhileStmtNode(parser, condition, then);
}

static P_Stmt* P_StmtDoWhile(P_Parser* parser) {
    parser->block_stmt_should_begin_scope = false;
    P_ScopeContext scope_context = P_BeginScope(parser, ScopeType_DoWhile);
    
    P_Stmt* then = P_Statement(parser);
    P_Consume(parser, TokenType_While, str_lit("Expected 'while'"));
    P_Consume(parser, TokenType_OpenParenthesis, str_lit("Expected ( after while\n"));
    P_Expr* condition = P_Expression(parser);
    if (!type_check(condition->ret_type, ValueType_Bool))
        report_error(parser, str_lit("While loop condition doesn't resolve to boolean\n"));
    P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after expression"));
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after )"));
    
    P_EndScope(parser, scope_context);
    parser->block_stmt_should_begin_scope = true;
    
    return P_MakeDoWhileStmtNode(parser, condition, then);
}

static P_Stmt* P_StmtExpression(P_Parser* parser) {
    P_Expr* expr = P_Expression(parser);
    P_Consume(parser, TokenType_Semicolon, str_lit("Expected ; after expression\n"));
    return P_MakeExpressionStmtNode(parser, expr);
}

static P_Stmt* P_Statement(P_Parser* parser) {
    if (!type_check(parser->function_body_ret, ValueType_Invalid)) {
        if (parser->scopetype_stack[parser->scopetype_tos - 1] == ScopeType_Switch ||
            parser->scopetype_stack[parser->scopetype_tos - 1] == ScopeType_Match) {
            
            if (P_Match(parser, TokenType_Case))
                return P_StmtCase(parser);
            else if (P_Match(parser, TokenType_Default))
                return P_StmtDefault(parser);
            
            report_error(parser, str_lit("Cannot Have Statementas Directly in a switch or match block\n"));
            return nullptr;
        } else {
            if (P_Match(parser, TokenType_OpenBrace))
                return P_StmtBlock(parser);
            else if (P_Match(parser, TokenType_Return))
                return P_StmtReturn(parser);
            else if (P_Match(parser, TokenType_If))
                return P_StmtIf(parser);
            else if (P_Match(parser, TokenType_While))
                return P_StmtWhile(parser);
            else if (P_Match(parser, TokenType_Do))
                return P_StmtDoWhile(parser);
            else if (P_Match(parser, TokenType_For))
                return P_StmtFor(parser);
            else if (P_Match(parser, TokenType_Switch))
                return P_StmtSwitch(parser);
            else if (P_Match(parser, TokenType_Match))
                return P_StmtMatch(parser);
            else if (P_Match(parser, TokenType_Break))
                return P_StmtBreak(parser);
            else if (P_Match(parser, TokenType_Continue))
                return P_StmtContinue(parser);
            else return P_StmtExpression(parser);
        }
    }
    
    report_error(parser, str_lit("Cannot Have Statements that exist outside of functions\n"));
    return nullptr;
}

static P_Stmt* P_Declaration(P_Parser* parser) {
    P_Stmt* s;
    
    // Consume semicolons
    while (P_Match(parser, TokenType_Semicolon));
    
    if (P_IsTypeToken(parser)) {
        P_ValueType type = P_ConsumeType(parser, str_lit("There's an error in the parser. (P_Declaration)\n"));
        b8 native = false;
        if (P_Match(parser, TokenType_Native)) native = true;
        P_Consume(parser, TokenType_Identifier, str_lit("Expected Identifier after variable type\n"));
        
        string name = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
        if (P_Match(parser, TokenType_OpenParenthesis)) {
            s = P_StmtFuncDecl(parser, type, name, native);
        } else {
            s = P_StmtVarDecl(parser, type, name);
        }
    } else if (P_Match(parser, TokenType_Struct)) {
        s = P_StmtStructureDecl(parser);
    } else if (P_Match(parser, TokenType_Enum)) {
        s = P_StmtEnumerationDecl(parser);
    } else if (P_Match(parser, TokenType_Import)) {
        if (parser->scope_depth == 0)
            s = P_StmtImport(parser);
        else {
            report_error(parser, str_lit("Cannot have an import statement in another declaration\n"));
            s = nullptr;
        }
    } else if (P_Match(parser, TokenType_EOF)) {
        report_error(parser, str_lit("Nothing to compile :|\n"));
        s = nullptr;
    } else
        s = P_Statement(parser);
    
    P_Sync(parser);
    return s;
}

//~ Pre-Parsing
static P_PreStmt* P_PreFuncDecl(P_Parser* parser, P_ValueType type, string name) {
    value_type_list params = {0};
    string_list param_names = {0};
    u32 arity = 0;
    b8 varargs = false;
    
    if (is_array(&type))
        report_error(parser, str_lit("Cannot Return Array type from function\n"));
    
    while (!P_Match(parser, TokenType_CloseParenthesis)) {
        if (P_Match(parser, TokenType_Dot)) {
            P_Consume(parser, TokenType_Dot, str_lit("Expected .. after .\n"));
            P_Consume(parser, TokenType_Dot, str_lit("Expected . after ..\n"));
            
            if (arity == 0)
                report_error(parser, str_lit("Only varargs function not allowed\n"));
            
            value_type_list_push(&parser->arena, &params, value_type_abs("..."));
            
            P_Consume(parser, TokenType_Identifier, str_lit("Expected param name\n"));
            varargs = true;
            string varargs_name = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
            string_list_push(&parser->arena, &param_names, varargs_name);
            
            P_Consume(parser, TokenType_CloseParenthesis, str_lit("Varargs can only be the last argument for a function\n"));
            arity++;
            break;
        }
        
        P_ValueType param_type = P_ConsumeType(parser, str_lit("Expected type\n"));
        if (is_array(&param_type))
            report_error(parser, str_lit("Cannot Pass Array type as parameter\n"));
        value_type_list_push(&parser->arena, &params, param_type);
        
        P_Consume(parser, TokenType_Identifier, str_lit("Expected param name\n"));
        string param_name = (string) { .str = (u8*)parser->previous.start, .size = parser->previous.length };
        string_list_push(&parser->arena, &param_names, param_name);
        
        arity++;
        if (!P_Match(parser, TokenType_Comma)) {
            P_Consume(parser, TokenType_CloseParenthesis, str_lit("Expected ) after parameters\n"));
            break;
        }
    }
    
    string additional = {0};
    if (varargs) additional = str_cat(&parser->arena, additional, str_lit("varargs"));
    
    string mangled = str_eq(str_lit("main"), name) ? name : P_FuncNameMangle(parser, name, varargs ? arity - 1 : arity, params, additional);
    
    u32 subset_match = 1024;
    func_entry_key key = (func_entry_key) { .name = name, .depth = 0 };
    func_entry_val* test = nullptr;
    if (!func_hash_table_get(&functions, key, &params, &test, &subset_match, true)) {
        func_entry_val* val = arena_alloc(&parser->arena, sizeof(func_entry_val));
        val->value = type;
        val->is_native = false;
        val->param_types = params;
        val->mangled_name = mangled;
        func_hash_table_set(&functions, key, val);
    } else
        report_error(parser, str_lit("Cannot redeclare function %.*s\n"), str_expand(name));
    
    P_PreStmt* func = P_MakePreFuncStmtNode(parser, type, mangled, arity, params, param_names);
    
    P_Consume(parser, TokenType_OpenBrace, str_lit("Expected {\n"));
    parser->scope_depth++;
    
    // Consume the function body :^)
    while (true) {
        if (P_Match(parser, TokenType_OpenBrace)) {
            parser->scope_depth++;
            continue;
        } else if (P_Match(parser, TokenType_CloseBrace)) {
            parser->scope_depth--;
            if (parser->scope_depth == 0)
                break;
            continue;
        }
        
        P_Advance(parser);
        if (P_Match(parser, TokenType_EOF)) {
            report_error(parser, str_lit("Unterminated function block\n"));
        }
    }
    
    return func;
}

static string read_file(M_Arena* arena, const char* path, b8* file_exists) {
    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        *file_exists = false;
        return (string) {0};
    } else {
        *file_exists = true;
    }
    
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    
    char* buffer = arena_alloc(arena, fileSize + 1);
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';
    
    fclose(file);
    return (string) { .str = (u8*)buffer, .size = bytesRead };
}

static P_PreStmt* P_PreStmtImport(P_Parser* parser) {
    P_Consume(parser, TokenType_StringLit, str_lit("Expected string after 'import'\n"));
    
    // String adjusted to be only the filename without quotes
    string name = { .str = (u8*)parser->previous.start + 1, .size = parser->previous.length - 2 };
    name = str_replace_all(&parser->arena, name, str_lit("\\"), str_lit("/"));
    
    u64 last_slash = str_find_last(parser->abspath, str_lit("/"), 0);
    
    string current_folder = (string) { .str = parser->abspath.str, .size = last_slash };
    string filename = str_cat(&parser->arena, current_folder, name);
    filename = fix_filepath(&global_arena, filename);
    
    if (string_list_contains(&imports, filename))
        return nullptr;
    
    // Adds a new parser to the children list.
    b8 file_exists;
    string content = read_file(&parser->arena, (const char*)filename.str, &file_exists);
    if (!file_exists) {
        // File doesn't exist
        report_error(parser, str_lit("File %.*s doesn't exist\n"), str_expand(name));
        return nullptr;
    }
    string_list_push(&global_arena, &imports, filename);
    P_Parser* child = P_AddChild(parser, content, name);
    child->abspath = filename;
    
    // Pre-Parse this file
    P_PreParse(child);
    P_PreStmt* prestmt_list = child->pre_root;
    
    // Add all the pre statements to the current list...
    P_PreStmt* curr = prestmt_list;
    while (curr != nullptr) {
        // TODO(voxel): Make Macro SLL Push
        if (parser->pre_end != nullptr) {
            parser->pre_end->next = curr;
            parser->pre_end = curr;
        } else {
            parser->pre_root = curr;
            parser->pre_end = curr;
        }
        curr = curr->next;
    }
    
    return nullptr;
}

static P_PreStmt* P_PreDeclaration(P_Parser* parser) {
    if (P_IsTypeToken(parser)) {
        P_ValueType type = P_ConsumeType(parser, str_lit("This is an error in the Parser. (P_PreDeclaration)\n"));
        if (P_Match(parser, TokenType_Native)) {
            while (!P_Match(parser, TokenType_CloseParenthesis)) {
                if (P_Match(parser, TokenType_EOF))
                    return nullptr;
                P_Advance(parser);
            }
            return nullptr;
        }
        
        if (!P_Match(parser, TokenType_Identifier)) return nullptr;
        
        string name = { .str = (u8*)parser->previous.start, .size = parser->previous.length };
        if (P_Match(parser, TokenType_OpenParenthesis)) {
            return P_PreFuncDecl(parser, type, name);
        }
    } else if (P_Match(parser, TokenType_Import)) {
        P_PreStmtImport(parser);
    }
    return nullptr;
}

//~ API
P_Parser* P_AddChild(P_Parser* parent, string source, string filename) {
    P_Parser* child = arena_alloc(&parent->arena, sizeof(P_Parser));
    P_Initialize(child, source, filename, false);
    child->parent = parent;
    
    if (parent->sub_count + 1 > parent->sub_cap) {
        void* prev = parent->sub;
        parent->sub_cap = GROW_CAPACITY(parent->sub_cap);
        parent->sub = arena_alloc(&parent->arena, parent->sub_cap * sizeof(P_Parser*));
        memmove(parent->sub, prev, parent->sub_count * sizeof(P_Parser*));
    }
    *(parent->sub + parent->sub_count) = child;
    parent->sub_count++;
    
    return child;
}

void P_PreParse(P_Parser* parser) {
    L_Initialize(&parser->lexer, parser->source);
    // Triple advance to pipe stuff to current and not just next and next_two
    P_Advance(parser);
    P_Advance(parser);
    P_Advance(parser);
    
    while (parser->current.type != TokenType_EOF && !parser->had_error) {
        P_PreStmt* tmp = P_PreDeclaration(parser);
        if (tmp != nullptr) {
            if (parser->pre_end != nullptr) {
                parser->pre_end->next = tmp;
                parser->pre_end = tmp;
            } else {
                parser->pre_root = tmp;
                parser->pre_end = tmp;
            }
        } else P_Advance(parser);
    }
}

void P_Parse(P_Parser* parser) {
    L_Initialize(&parser->lexer, parser->source);
    // Triple advance to pipe stuff to current and not just next and next_two
    P_Advance(parser);
    P_Advance(parser);
    P_Advance(parser);
    
    P_Stmt* root = P_Declaration(parser);
    if (parser->end != nullptr) {
        parser->end->next = root;
        parser->end = root;
    } else {
        parser->root = root;
        parser->end = root;
    }
    
    while (parser->current.type != TokenType_EOF && !parser->had_error) {
        P_Stmt* tmp = P_Declaration(parser);
        if (parser->end != nullptr) {
            parser->end->next = tmp;
            parser->end = tmp;
        } else {
            parser->root = tmp;
            parser->end = tmp;
        }
    }
    
    // Only check for main function if it is the entry point
    if (parser->parent == nullptr) {
        u32 subset_match = 1024;
        func_entry_val* v = nullptr;
        value_type_list noargs = (value_type_list){0};
        if (!func_hash_table_get(&functions, (func_entry_key) { .name = str_lit("main"), .depth = 0 }, &noargs, &v, &subset_match, true)) {
            report_error(parser, str_lit("No main function definition found\n"));
        }
    }
}

void P_GlobalInit() {
    arena_init(&global_arena);
    var_hash_table_init(&variables);
    func_hash_table_init(&functions);
    types_init(&global_arena);
    type_array_init(&types);
    imports = (string_list){0};
    imports_parsing = (string_list){0};
    char* buffer = arena_alloc(&global_arena, PATH_MAX);
    get_cwd(buffer, PATH_MAX);
    cwd = (string){ .str = (u8*)buffer, .size = strlen(buffer) };
}

void P_Initialize(P_Parser* parser, string source, string filename, b8 is_root) {
    if (!parser->initialized) {
        arena_init(&parser->arena);
    }
    parser->source = source;
    parser->filename = filename;
    if (is_root) {
        parser->abspath = str_cat(&parser->arena, cwd, str_lit("/"));
        parser->abspath = str_cat(&parser->arena, parser->abspath, parser->filename);
        parser->abspath = str_replace_all(&parser->arena, parser->abspath, str_lit("\\"), str_lit("/"));
    }
    parser->pre_root = nullptr;
    parser->pre_end = nullptr;
    parser->root = nullptr;
    parser->next = (L_Token) {0};
    parser->current = (L_Token) {0};
    parser->previous = (L_Token) {0};
    parser->had_error = false;
    parser->panik_mode = false;
    parser->scope_depth = 0;
    parser->function_body_ret = ValueType_Invalid;
    parser->switch_type = ValueType_Invalid;
    parser->is_directly_in_func_body = false;
    parser->encountered_return = false;
    parser->all_code_paths_return = false;
    parser->lambda_number = 0;
    parser->scopetype_stack = arena_alloc(&parser->arena, 1024 * sizeof(P_ScopeType));
    parser->scopetype_tos = 0;
    parser->current_function = (string) {0};
    parser->block_stmt_should_begin_scope = true;
    parser->is_in_private_scope = false;
    parser->parent = nullptr;
    if (!parser->initialized) {
        parser->sub = nullptr;
        parser->sub_cap = 0;
        parser->sub_count = 0;
    }
    parser->initialized = true;
}

void P_Free(P_Parser* parser) {
    for (u32 i = 0; i < parser->sub_count; i++)
        P_Free(parser->sub[i]);
    arena_free(&parser->arena);
}

void P_GlobalFree() {
    var_hash_table_free(&variables);
    func_hash_table_free(&functions);
    type_array_free(&types);
    arena_free(&global_arena);
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
            printf("%.*s [String]\n", str_expand(expr->op.string_lit));
        } break;
        
        case ExprType_CharLit: {
            printf("%.*s [Char]\n", str_expand(expr->op.char_lit));
        } break;
        
        case ExprType_BoolLit: {
            printf("%s [Bool]\n", expr->op.bool_lit ? "True" : "False");
        } break;
        
        case ExprType_Assignment: {
            P_PrintExprAST_Indent(arena, expr->op.assignment.name, indent + 1);
            printf("=\n");
            P_PrintExprAST_Indent(arena, expr->op.assignment.value, indent + 1);
        } break;
        
        case ExprType_Cast: {
            printf("(%.*s) Cast\n", str_expand(expr->ret_type.full_type));
            P_PrintExprAST_Indent(arena, expr->op.cast, indent + 1);
        } break;
        
        case ExprType_Variable: {
            printf("%.*s\n", str_expand(expr->op.variable));
        } break;
        
        case ExprType_Typename: {
            printf("%.*s [Typename]\n", str_expand(expr->op.typename.full_type));
        } break;
        
        case ExprType_Funcname: {
            printf("%.*s [Funcname]\n", str_expand(expr->op.funcname));
        } break;
        
        case ExprType_Lambda: {
            printf("%.*s [Lambda]\n", str_expand(expr->op.lambda));
        } break;
        
        case ExprType_Unary: {
            printf("%.*s\n", str_expand(L__get_string_from_type__(expr->op.unary.operator)));
            P_PrintExprAST_Indent(arena, expr->op.unary.operand, indent + 1);
        } break;
        
        case ExprType_Nullptr: {
            printf("nullptr\n");
        } break;
        
        case ExprType_ArrayLit: {
            printf("Array: \n");
            for (u32 i = 0; i < expr->op.array.count; i++) {
                P_PrintExprAST_Indent(arena, expr->op.array.elements[i], indent + 1);
            }
        } break;
        
        case ExprType_Binary: {
            printf("%.*s\n", str_expand(L__get_string_from_type__(expr->op.binary.operator)));
            P_PrintExprAST_Indent(arena, expr->op.binary.left, indent + 1);
            P_PrintExprAST_Indent(arena, expr->op.binary.right, indent + 1);
        } break;
        
        case ExprType_FuncCall: {
            printf("%.*s()\n", str_expand(expr->op.func_call.name));
            for (u32 i = 0; i < expr->op.func_call.call_arity; i++)
                P_PrintExprAST_Indent(arena, expr->op.func_call.params[i], indent + 1);
        } break;
        
        case ExprType_Call: {
            printf("()\n");
            P_PrintExprAST_Indent(arena, expr->op.call.left, indent + 1);
            for (u32 i = 0; i < expr->op.call.call_arity; i++)
                P_PrintExprAST_Indent(arena, expr->op.call.params[i], indent + 1);
        } break;
        
        case ExprType_Index: {
            printf("[Index]");
            P_PrintExprAST_Indent(arena, expr->op.index.operand, indent + 1);
            P_PrintExprAST_Indent(arena, expr->op.index.index, indent + 1);
        } break;
        
        case ExprType_Addr: {
            printf("[Get Address]\n");
            P_PrintExprAST_Indent(arena, expr->op.addr, indent + 1);
        } break;
        
        case ExprType_Deref: {
            printf("[Deref]\n");
            P_PrintExprAST_Indent(arena, expr->op.deref, indent + 1);
        } break;
        
        case ExprType_Dot: {
            printf(".%.*s [Member Access]\n", str_expand(expr->op.dot.right));
            P_PrintExprAST_Indent(arena, expr->op.dot.left, indent + 1);
        } break;
        
        case ExprType_EnumDot: {
            printf("%.*s.%.*s [Enum Access]\n", str_expand(expr->op.enum_dot.left), str_expand(expr->op.enum_dot.right));
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
            printf("%.*s: %.*s\n", str_expand(stmt->op.var_decl.name), str_expand(stmt->op.var_decl.type.full_type));
        } break;
        
        case StmtType_VarDeclAssign: {
            printf("Variable Declaration:\n");
            for (u8 i = 0; i < indent; i++)
                printf("  ");
            printf("%.*s: %.*s = \n", str_expand(stmt->op.var_decl_assign.name), str_expand(stmt->op.var_decl_assign.type.full_type));
            P_PrintExprAST_Indent(arena, stmt->op.var_decl_assign.val, indent + 1);
        } break;
        
        case StmtType_FuncDecl: {
            printf("Function Declaration: %.*s: %.*s\n", str_expand(stmt->op.func_decl.name), str_expand(stmt->op.func_decl.type.full_type));
            if (stmt->op.func_decl.block != nullptr)
                P_PrintAST_Indent(arena, stmt->op.func_decl.block, indent + 1);
        } break;
        
        case StmtType_StructDecl: {
            printf("Struct Declaration: %s\n", stmt->op.struct_decl.name.str);
            string_list_node* curr = stmt->op.struct_decl.member_names.first;
            value_type_list_node* type = stmt->op.struct_decl.member_types.first;
            for (u32 i = 0; i < stmt->op.struct_decl.member_count; i++) {
                for (u8 idt = 0; idt < indent + 1; idt++)
                    printf("  ");
                printf("%.*s: %.*s\n", str_node_expand(curr), str_expand(type->type.full_type));
                
                curr = curr->next;
                type = type->next;
            }
        } break;
        
        case StmtType_EnumDecl: {
            printf("Enum Declaration: %s\n", stmt->op.enum_decl.name.str);
            string_list_node* curr = stmt->op.enum_decl.member_names.first;
            for (u32 i = 0; i < stmt->op.enum_decl.member_count; i++) {
                for (u8 idt = 0; idt < indent + 1; idt++)
                    printf("  ");
                printf("%.*s\n", str_node_expand(curr));
                
                curr = curr->next;
            }
        } break;
        
        case StmtType_For: {
            printf("For Loop:\n");
            if (stmt->op.for_s.init != nullptr)
                P_PrintAST_Indent(arena, stmt->op.for_s.init, indent + 1);
            P_PrintExprAST_Indent(arena, stmt->op.for_s.condition, indent + 1);
            if (stmt->op.for_s.loopexec != nullptr)
                P_PrintExprAST_Indent(arena, stmt->op.for_s.loopexec, indent + 1);
            P_PrintAST_Indent(arena, stmt->op.for_s.then, indent + 1);
        } break;
        
        case StmtType_While: {
            printf("While Loop:\n");
            P_PrintExprAST_Indent(arena, stmt->op.while_s.condition, indent + 1);
            P_PrintAST_Indent(arena, stmt->op.while_s.then, indent + 1);
        } break;
        
        
        case StmtType_Switch: {
            printf("Switch:\n");
            P_PrintExprAST_Indent(arena, stmt->op.switch_s.switched, indent + 1);
            P_PrintAST_Indent(arena, stmt->op.switch_s.then, indent + 1);
        } break;
        
        case StmtType_Match: {
            printf("Match:\n");
            P_PrintExprAST_Indent(arena, stmt->op.match_s.matched, indent + 1);
            P_PrintAST_Indent(arena, stmt->op.match_s.then, indent + 1);
        } break;
        
        case StmtType_Case: {
            printf("Case:\n");
            P_PrintExprAST_Indent(arena, stmt->op.case_s.value, indent + 1);
            P_PrintAST_Indent(arena, stmt->op.case_s.then, indent + 1);
        } break;
        
        case StmtType_MatchCase: {
            printf("Matched Case:\n");
            P_PrintExprAST_Indent(arena, stmt->op.case_s.value, indent + 1);
            P_PrintAST_Indent(arena, stmt->op.case_s.then, indent + 1);
        } break;
        
        case StmtType_Default: {
            printf("Default:\n");
            P_PrintAST_Indent(arena, stmt->op.default_s.then, indent + 1);
        } break;
        
        case StmtType_MatchDefault: {
            printf("Matched Default:\n");
            P_PrintAST_Indent(arena, stmt->op.mdefault_s.then, indent + 1);
        } break;
        
        case StmtType_Break: {
            printf("Break\n");
        } break;
        
        case StmtType_Continue: {
            printf("Continue\n");
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
