/*
 TODOS(voxel):
- Exit Function for the compiler which cleans everything up
- Better Error Output. Should show line with ~~~~^ underneath offending token
*/

#include "parser.h"
#include "parser_data.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

Array_Impl(type_array, P_Type*);
Array_Impl(node_array, AstNode*);

static void P_Report(P_Parser* parser, L_Token token, const char* stage, const char* err, ...) {
    if (parser->errored) return;
    if (parser->error_count > 20) exit(-1);
    fprintf(stderr, "%s Error:%d:%d: ", stage, token.line, token.column);
    va_list va;
    va_start(va, err);
    vfprintf(stderr, err, va);
    va_end(va);
    parser->errored = true;
}

#define P_ReportLexError(parser, error, ...) P_Report(parser, parser->next, "Lexer", error, __VA_ARGS__)
#define P_ReportParseError(parser, error, ...) P_Report(parser, parser->next, "Parser", error, __VA_ARGS__)

//~ Parser Snapshot stuff
P_ParserSnap P_TakeSnapshot(P_Parser* parser) {
    P_ParserSnap snap = {0};
    snap.lexer = *parser->lexer;
    snap.prev  =  parser->prev;
    snap.curr  =  parser->curr;
    snap.next  =  parser->next;
    return snap;
}

void P_ApplySnapshot(P_Parser* parser, P_ParserSnap snap) {
    *parser->lexer = snap.lexer;
    parser->prev   = snap.prev;
    parser->curr   = snap.curr;
    parser->next   = snap.next;
}

//~ Node Allocation
static AstNode* P_AllocNode(P_Parser* parser, P_NodeType type) {
    AstNode* node = pool_alloc(&parser->node_pool);
    node->type = type;
    return node;
}

static AstNode* error_node_instance;
static AstNode* P_AllocErrorNode(P_Parser* parser) {
    if (!error_node_instance)
        return P_AllocNode(parser, NodeType_Error);
    return error_node_instance;
}

//- Expression Node Allocation 
static AstNode* P_AllocIntLitNode(P_Parser* parser, i64 val) {
    AstNode* node = P_AllocNode(parser, NodeType_IntLit);
    node->IntLit = val;
    return node;
}

static AstNode* P_AllocGlobalStringNode(P_Parser* parser, L_Token token, string val) {
    AstNode* node = P_AllocNode(parser, NodeType_GlobalString);
    node->id = token;
    node->GlobalString.value = val;
    return node;
}

static AstNode* P_AllocIdentNode(P_Parser* parser, L_Token val) {
    AstNode* node = P_AllocNode(parser, NodeType_Ident);
    node->Ident = val.lexeme;
    node->id = val;
    return node;
}

static AstNode* P_AllocUnaryNode(P_Parser* parser, AstNode* operand, L_Token op) {
    AstNode* node = P_AllocNode(parser, NodeType_Unary);
    node->Unary.expr = operand;
    node->id = op;
    return node;
}

static AstNode* P_AllocBinaryNode(P_Parser* parser, AstNode* lhs, AstNode* rhs, L_Token op) {
    AstNode* node = P_AllocNode(parser, NodeType_Binary);
    node->Binary.left  = lhs;
    node->Binary.right = rhs;
    node->id = op;
    return node;
}

static AstNode* P_AllocGroupNode(P_Parser* parser, AstNode* expr, L_Token token) {
    AstNode* node = P_AllocNode(parser, NodeType_Group);
    node->Group = expr;
    node->id = token;
    return node;
}

static AstNode* P_AllocLambdaNode(P_Parser* parser, P_Type* function_type, L_Token func, AstNode* body) {
    AstNode* node = P_AllocNode(parser, NodeType_Lambda);
    node->Lambda.function_type = function_type;
    node->id = func;
    node->Lambda.body = body;
    return node;
}

//- Statement Node Allocation
static AstNode* P_AllocReturnNode(P_Parser* parser, AstNode* expr, L_Token token) {
    AstNode* node = P_AllocNode(parser, NodeType_Return);
    node->Return = expr;
    node->id = token;
    return node;
}

static AstNode* P_AllocBlockNode(P_Parser* parser, P_Scope scope, AstNode** statements, u32 count, L_Token token) {
    AstNode* node = P_AllocNode(parser, NodeType_Block);
    node->Block.scope = scope;
    node->Block.statements = statements;
    node->Block.count = count;
    node->id = token;
    return node;
}

static AstNode* P_AllocAssignNode(P_Parser* parser, L_Token name, AstNode* value) {
    AstNode* node = P_AllocNode(parser, NodeType_Assign);
    node->id = name;
    node->Assign.value = value;
    return node;
}

static AstNode* P_AllocVarDeclNode(P_Parser* parser, P_Type* type, L_Token name, AstNode* value) {
    AstNode* node = P_AllocNode(parser, NodeType_VarDecl);
    node->VarDecl.type = type;
    node->id = name;
    node->VarDecl.value = value;
    return node;
}

//- Type Allocation
static P_Type* P_AllocType(P_Parser* parser, P_BasicType type) {
    P_Type* node = pool_alloc(&parser->type_pool);
    node->type = type;
    return node;
}

static P_Type* P_AllocErrorType(P_Parser* parser) {
    P_Type* node = P_AllocType(parser, BasicType_Invalid);
    return node;
}

static P_Type* P_AllocIntType(P_Parser* parser, L_Token tok) {
    P_Type* type = P_AllocType(parser, BasicType_Integer);
    type->token = tok;
    return type;
}

static P_Type* P_AllocVoidType(P_Parser* parser, L_Token tok) {
    P_Type* type = P_AllocType(parser, BasicType_Void);
    type->token = tok;
    return type;
}

static P_Type* P_AllocFunctionType(P_Parser* parser, P_Type* return_type, P_Type** param_types, string* param_names, u32 arity, L_Token func_token) {
    P_Type* type = P_AllocType(parser, BasicType_Function);
    type->token = func_token;
    type->function.return_type = return_type;
    type->function.param_types = param_types;
    type->function.param_names = param_names;
    type->function.arity = arity;
    return type;
}

//~ Token Helpers
static void P_Advance(P_Parser* parser) {
    parser->prev = parser->curr;
    parser->curr = parser->next;
    while (true) {
        parser->next = L_LexToken(parser->lexer);
        if (parser->next.type != TokenType_Error) break;
        P_ReportLexError(parser, "Unexpected Token: %.*s\n", str_expand(parser->next.lexeme));
        parser->errored = false;
    }
}

static void P_Eat(P_Parser* parser, L_TokenType type) {
    if (parser->curr.type != type) {
        P_ReportParseError(parser, "Invalid Token. Expected %.*s got %.*s\n", str_expand(L_GetTypeName(type)), str_expand(L_GetTypeName(parser->curr.type)));
    }
    P_Advance(parser);
}

static b8 P_Match(P_Parser* parser, L_TokenType type) {
    if (parser->curr.type == type) {
        P_Advance(parser);
        return true;
    }
    return false;
}

static b8 P_IsType(P_Parser* parser) {
    b8 ret;
    P_ParserSnap snap = P_TakeSnapshot(parser);
    
    switch (parser->curr.type) {
        case TokenType_Int: ret = true; break;
        case TokenType_Void: ret = true; break;
        case TokenType_Func: ret = true; break;
        default: ret = false; break;
    }
    
    P_ApplySnapshot(parser, snap);
    return ret;
}

static P_Type* P_EatType(P_Parser* parser) {
    switch (parser->curr.type) {
        case TokenType_Int: P_Advance(parser); return P_AllocIntType(parser, parser->prev);
        case TokenType_Void: P_Advance(parser); return P_AllocVoidType(parser, parser->prev);
        
        case TokenType_Func: {
            P_Advance(parser);
            L_Token func_token = parser->prev;
            P_Eat(parser, TokenType_OpenParenthesis);
            
            string_array temp_param_names = {0};
            type_array temp_param_types = {0};
            
            u32 arity = 0;
            
            while (!P_Match(parser, TokenType_CloseParenthesis)) {
                type_array_add(&temp_param_types, P_EatType(parser));
                
                if (P_Match(parser, TokenType_Identifier)) {
                    string name = parser->prev.lexeme;
                    string_array_add(&temp_param_names, name);
                } else {
                    string_array_add(&temp_param_names, str_lit(""));
                }
                
                arity++;
                
                if (!P_Match(parser, TokenType_Comma)) {
                    P_Eat(parser, TokenType_CloseParenthesis);
                    break;
                }
            }
            
            // TODO(voxel): please stop being lazy and implement arena_raise already
            string* param_names = arena_alloc(&parser->arena, sizeof(string) * arity);
            memcpy(param_names, temp_param_names.elems, sizeof(string) * arity);
            P_Type** param_types = arena_alloc(&parser->arena, sizeof(AstNode*) * arity);
            memcpy(param_types, temp_param_types.elems, sizeof(P_Type*) * arity);
            
            P_Type* return_type = nullptr;
            if (P_Match(parser, TokenType_ThinArrow))
                return_type = P_EatType(parser);
            if (!return_type) return_type = P_AllocVoidType(parser, (L_Token){0});
            
            string_array_free(&temp_param_names);
            type_array_free(&temp_param_types);
            return P_AllocFunctionType(parser, return_type, param_types, param_names, arity, func_token);
        }
        
        default: P_ReportParseError(parser, "Could not consume type. Got %.*s token\n", str_expand(L_GetTypeName(parser->curr.type)));
    }
    return P_AllocErrorType(parser);
}

//~ Expressions
static AstNode* P_ExprUnary(P_Parser* parser, b8 rhs);
static AstNode* P_Statement(P_Parser* parser);
static AstNode* P_Expression(P_Parser* parser, Prec prec_in, b8 is_rhs);

static AstNode* P_ExprIntegerLiteral(P_Parser* parser) {
    i64 value = atoll((const char*)parser->prev.lexeme.str);
    return P_AllocIntLitNode(parser, value);
}

static AstNode* P_ExprIdent(P_Parser* parser) {
    return P_AllocIdentNode(parser, parser->prev);
}

static AstNode* P_ExprStringLit(P_Parser* parser) {
    string value = { .str = parser->prev.lexeme.str + 1, .size = parser->prev.lexeme.size - 2 };
    return P_AllocGlobalStringNode(parser, parser->prev, value);
}

static AstNode* P_ExprUnaryNum(P_Parser* parser) {
    L_Token op = parser->prev;
    AstNode* expr = P_ExprUnary(parser, false);
    return P_AllocUnaryNode(parser, expr, op);
}

static AstNode* P_ExprUnary(P_Parser* parser, b8 is_rhs) {
    switch (parser->curr.type) {
        case TokenType_IntLit: P_Advance(parser); return P_ExprIntegerLiteral(parser);
        case TokenType_CstringLit: P_Advance(parser); return P_ExprStringLit(parser);
        case TokenType_Identifier: P_Advance(parser); return P_ExprIdent(parser);
        case TokenType_OpenParenthesis: {
            P_Advance(parser);
            L_Token tok = parser->prev;
            AstNode* in = P_Expression(parser, Prec_Invalid, false);
            P_Eat(parser, TokenType_CloseParenthesis);
            return P_AllocGroupNode(parser, in, tok);
        }
        
        case TokenType_Plus:
        case TokenType_Minus:
        case TokenType_Tilde:
        P_Advance(parser); return P_ExprUnaryNum(parser);
        
        case TokenType_Func: {
            L_Token func_token = parser->curr;
            P_Type* func_type = P_EatType(parser);
            AstNode* body = P_Statement(parser);
            return P_AllocLambdaNode(parser, func_type, func_token, body);
        }
        
        default: {
            P_Advance(parser);
            if (is_rhs)
                P_ReportParseError(parser, "Invalid Expression for Right Hand Side of %.*s\n", str_expand(parser->prev.lexeme));
            else P_ReportParseError(parser, "Invalid Unary Expression %.*s\n", str_expand(parser->prev.lexeme));
            return P_AllocErrorNode(parser);
        }
    }
    return P_AllocErrorNode(parser);
}

static AstNode* P_Expression(P_Parser* parser, Prec prec_in, b8 is_rhs) {
    AstNode* lhs = P_ExprUnary(parser, is_rhs);
    if (lhs->type == NodeType_Error) return P_AllocErrorNode(parser);
    AstNode* rhs;
    
    if (infix_expr_precs[parser->curr.type] != Prec_Invalid) {
        P_Advance(parser);
        while (true) {
            L_Token op = parser->prev;
            if (infix_expr_precs[op.type] == Prec_Invalid) break;
            
            if (infix_expr_precs[op.type] >= prec_in) {
                rhs = P_Expression(parser, infix_expr_precs[op.type] + 1, true);
                if (rhs->type == NodeType_Error) return P_AllocErrorNode(parser);
                lhs = P_AllocBinaryNode(parser, lhs, rhs, op);
            } else break;
        }
    }
    
    return lhs;
}

static AstNode* P_Statement(P_Parser* parser) {
    if (P_Match(parser, TokenType_Return)) {
        L_Token tok = parser->prev;
        AstNode* expr = nullptr;
        if (!P_Match(parser, TokenType_Semicolon))
            expr = P_Expression(parser, Prec_Invalid, false);
        parser->errored = false;
        return P_AllocReturnNode(parser, expr, tok);
    } else if (P_Match(parser, TokenType_Identifier)) {
        L_Token name = parser->prev;
        
        if (P_Match(parser, TokenType_Colon)) {
            P_Type* type = P_EatType(parser);
            AstNode* value = nullptr;
            if (P_Match(parser, TokenType_Equal)) value = P_Expression(parser, Prec_Invalid, false);
            
            // Eat semicolon only if value is not a lambda
            if (value) {
                if (value->type != NodeType_Lambda) {
                    P_Eat(parser, TokenType_Semicolon);
                }
            } else P_Eat(parser, TokenType_Semicolon);
            
            return P_AllocVarDeclNode(parser, type, name, value);
        }
        
        P_Eat(parser, TokenType_Equal);
        AstNode* value = P_Expression(parser, Prec_Invalid, false);
        P_Eat(parser, TokenType_Semicolon);
        parser->errored = false;
        return P_AllocAssignNode(parser, name, value);
    } else if (P_Match(parser, TokenType_OpenBrace)) {
        L_Token tok = parser->prev;
        node_array temp_statements = {0};
        u32 count = 0;
        
        while (!P_Match(parser, TokenType_CloseBrace)) {
            AstNode* stmt = P_Statement(parser);
            node_array_add(&temp_statements, stmt);
            if (stmt->type == NodeType_Error) P_Advance(parser);
            count++;
        }
        
        // NOTE(voxel): please stop being lazy and implement arena_raise already
        AstNode** statements = arena_alloc(&parser->arena, sizeof(AstNode*) * count);
        memcpy(statements, temp_statements.elems, sizeof(AstNode*) * count);
        
        node_array_free(&temp_statements);
        P_Scope scope = { .type = ScopeType_None };
        parser->errored = false;
        return P_AllocBlockNode(parser, scope, statements, count, tok);
    }
    return P_AllocErrorNode(parser);
}

void P_Init(P_Parser* parser, L_Lexer* lexer) {
    parser->lexer = lexer;
    pool_init(&parser->node_pool, sizeof(AstNode));
    pool_init(&parser->type_pool, sizeof(P_Type));
    arena_init(&parser->arena);
    
    P_Advance(parser); // Next
    P_Advance(parser); // Curr
}

AstNode* P_Parse(P_Parser* parser) {
    if (parser->curr.type == TokenType_EOF) return P_AllocErrorNode(parser);
    return P_Statement(parser);
}

void P_Free(P_Parser* parser) {
    arena_free(&parser->arena);
    pool_free(&parser->node_pool);
    pool_free(&parser->type_pool);
}
