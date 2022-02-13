/*
 TODOS(voxel):
- Exit Function for the compiler which cleans everything up
- Better Error Output. Should show line with ~~~~^ underneath offencding token
*/

#include "parser.h"
#include "parser_data.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static void P_Report(P_Parser* parser, L_Token token, const char* stage, const char* err, ...) {
    if (parser->errored) return;
    if (parser->error_count > 20) exit(-1);
    fprintf(stderr, "%s Error:%d: ", stage, token.line);
    va_list va;
    va_start(va, err);
    vfprintf(stderr, err, va);
    va_end(va);
    parser->errored = true;
}

#define P_ReportLexError(parser, error, ...) P_Report(parser, parser->next, "Lexer", error, __VA_ARGS__);
#define P_ReportParseError(parser, error, ...) P_Report(parser, parser->next, "Parser", error, __VA_ARGS__);

//~ Token Helpers
static void P_Advance(P_Parser* parser) {
    parser->prev = parser->curr;
    parser->curr = parser->next;
    while (true) {
        parser->next = L_LexToken(parser->lexer);
        if (parser->next.type != TokenType_Error) break;
        P_ReportLexError(parser, "Unexpected Token\n");
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

//~ Node Allocation
static AstNode* P_AllocNode(P_Parser* parser, P_NodeType type) {
    AstNode* node = pool_alloc(&parser->node_pool);
    node->type = type;
    return node;
}

static AstNode* P_AllocIntLitNode(P_Parser* parser, i64 val) {
    AstNode* node = P_AllocNode(parser, NodeType_IntLit);
    node->IntLit = val;
    return node;
}

//~ Expressions
static AstNode* P_ExprUnary(P_Parser* parser);

static AstNode* P_ExprIntegerLiteral(P_Parser* parser) {
    i64 value = atoll(parser->prev.start);
    return P_AllocIntLitNode(parser, value);
}

static AstNode* P_ExprUnary(P_Parser* parser) {
    P_Advance(parser);
    switch (parser->prev.type) {
        case TokenType_IntLit: return P_ExprIntegerLiteral(parser);
        default: {
            P_ReportParseError(parser, "Invalid Expression for %.*s\n", parser->prev.start, parser->prev.length);
            return nullptr;
        }
    }
    return nullptr;
}

static AstNode* P_Expression(P_Parser* parser) {
    AstNode* lhs = P_ExprUnary(parser);
    return lhs;
}

void P_Init(P_Parser* parser, L_Lexer* lexer) {
    parser->lexer = lexer;
    pool_init(&parser->node_pool, sizeof(AstNode));
    
    P_Advance(parser); // Next
    P_Advance(parser); // Curr
}

AstNode* P_Parse(P_Parser* parser) {
    return P_Expression(parser);
}

void P_Free(P_Parser* parser) {
    pool_free(&parser->node_pool);
}
