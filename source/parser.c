/*
 TODOS(voxel):
- Exit Function for the compiler which cleans everything up
- Better Error Output. Should show line with ~~~~^ underneath offending token
*/

#include "parser.h"
#include "parser_data.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

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

static AstNode* P_AllocErrorNode(P_Parser* parser) {
    AstNode* node = P_AllocNode(parser, NodeType_Error);
    return node;
}

static AstNode* P_AllocIntLitNode(P_Parser* parser, i64 val) {
    AstNode* node = P_AllocNode(parser, NodeType_IntLit);
    node->IntLit = val;
    return node;
}

static AstNode* P_AllocGlobalStringNode(P_Parser* parser, string val) {
    AstNode* node = P_AllocNode(parser, NodeType_GlobalString);
    node->GlobalString = val;
    return node;
}

static AstNode* P_AllocIdentNode(P_Parser* parser, string val) {
    AstNode* node = P_AllocNode(parser, NodeType_Ident);
    node->Ident = val;
    return node;
}

static AstNode* P_AllocUnaryNode(P_Parser* parser, AstNode* operand, L_Token op) {
    AstNode* node = P_AllocNode(parser, NodeType_Unary);
    node->Unary.expr = operand;
    node->Unary.op   = op;
    return node;
}

static AstNode* P_AllocBinaryNode(P_Parser* parser, AstNode* lhs, AstNode* rhs, L_Token op) {
    AstNode* node = P_AllocNode(parser, NodeType_Binary);
    node->Binary.left  = lhs;
    node->Binary.right = rhs;
    node->Binary.op    = op;
    return node;
}

static AstNode* P_AllocReturnNode(P_Parser* parser, AstNode* expr) {
    AstNode* node = P_AllocNode(parser, NodeType_Return);
    node->Return = expr;
    return node;
}

static AstNode* P_AllocVarDeclNode(P_Parser* parser, L_Token type, L_Token name, AstNode* value) {
    AstNode* node = P_AllocNode(parser, NodeType_VarDecl);
    node->VarDecl.type = type;
    node->VarDecl.name = name;
    node->VarDecl.value = value;
    return node;
}

//~ Expressions
static AstNode* P_ExprUnary(P_Parser* parser, b8 rhs);

static AstNode* P_ExprIntegerLiteral(P_Parser* parser) {
    i64 value = atoll(parser->prev.start);
    return P_AllocIntLitNode(parser, value);
}

static AstNode* P_ExprIdent(P_Parser* parser) {
    string value = { .str = (u8*) parser->prev.start, .size = parser->prev.length };
    return P_AllocIdentNode(parser, value);
}

static AstNode* P_ExprStringLit(P_Parser* parser) {
    string value = { .str = (u8*) parser->prev.start + 1, .size = parser->prev.length - 2 };
    return P_AllocGlobalStringNode(parser, value);
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
        
        case TokenType_Plus:
        case TokenType_Minus:
        case TokenType_Tilde:
        P_Advance(parser); return P_ExprUnaryNum(parser);
        
        default: {
            if (is_rhs)
                P_ReportParseError(parser, "Invalid Expression for Right Hand Side of %.*s\n", parser->prev.length, parser->prev.start);
            else P_ReportParseError(parser, "Invalid Expression for Left Hand Side of %.*s\n", parser->prev.length, parser->prev.start);
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
        AstNode* expr = P_Expression(parser, Prec_Invalid, false);
        return P_AllocReturnNode(parser, expr);
    } else if (P_Match(parser, TokenType_Int)) {
        L_Token type = parser->prev;
        P_Eat(parser, TokenType_Identifier);
        L_Token name = parser->prev;
        AstNode* value = nullptr;
        if (P_Match(parser, TokenType_Equal))
            value = P_Expression(parser, Prec_Invalid, false);
        return P_AllocVarDeclNode(parser, type, name, value);
    }
    return P_AllocErrorNode(parser);
}

void P_Init(P_Parser* parser, L_Lexer* lexer) {
    parser->lexer = lexer;
    pool_init(&parser->node_pool, sizeof(AstNode));
    
    P_Advance(parser); // Next
    P_Advance(parser); // Curr
}

AstNode* P_Parse(P_Parser* parser) {
    if (parser->curr.type == TokenType_EOF) return P_AllocErrorNode(parser);
    return P_Statement(parser);
}

void P_Free(P_Parser* parser) {
    pool_free(&parser->node_pool);
}

void PrintAst_Indent(AstNode* node, u32 indent) {
    for (u32 k = 0; k < indent; k++) printf(" ");
    
    switch (node->type) {
        case NodeType_Error: {
            printf("ERROR NODE\n");
        } break;
        
        case NodeType_Ident: {
            printf("Identifier [%.*s]\n", str_expand(node->Ident));
        } break;
        
        case NodeType_IntLit: {
            printf("%lld\n", node->IntLit);
        } break;
        
        case NodeType_GlobalString: {
            printf("String Literal [%.*s]\n", str_expand(node->GlobalString));
        } break;
        
        case NodeType_Unary: {
            printf("Unary %.*s\n", str_expand(L_GetTypeName(node->Unary.op.type)));
            PrintAst_Indent(node->Unary.expr, indent + 1);
        } break;
        
        case NodeType_Binary: {
            printf("Binary %.*s\n", str_expand(L_GetTypeName(node->Binary.op.type)));
            PrintAst_Indent(node->Binary.left, indent + 1);
            PrintAst_Indent(node->Binary.right, indent + 1);
        } break;
        
        case NodeType_Return: {
            printf("Return\n");
            PrintAst_Indent(node->Return, indent + 1);
        } break;
        
        case NodeType_VarDecl: {
            if (node->VarDecl.value) {
                printf("Assign to %.*s\n", node->VarDecl.name.length, node->VarDecl.name.start);
                PrintAst_Indent(node->VarDecl.value, indent + 1);
            } else {
                printf("Declare %.*s\n", node->VarDecl.name.length, node->VarDecl.name.start);
            }
        } break;
    }
}

void PrintAst(AstNode* node) {
    PrintAst_Indent(node, 0);
    printf("\n");
}
