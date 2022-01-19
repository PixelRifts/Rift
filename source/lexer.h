/* date = October 7th 2021 11:58 am */

#ifndef LEXER_H
#define LEXER_H

#include "defines.h"
#include "str.h"

typedef u32 L_TokenType;
enum {
    TokenType_Error, TokenType_EOF, TokenType_Whitespace,
    
    TokenType_Identifier, TokenType_CstringLit,
    TokenType_IntLit, TokenType_FloatLit, TokenType_DoubleLit, TokenType_CharLit,
    TokenType_LongLit,
    
    TokenType_Plus, TokenType_Minus, TokenType_Star, TokenType_Slash,
    TokenType_Ampersand, TokenType_Pipe,
    TokenType_Tilde, TokenType_Bang,
    TokenType_BangEqual, TokenType_EqualEqual,
    TokenType_Less, TokenType_Greater,
    TokenType_LessEqual, TokenType_GreaterEqual,
    TokenType_AmpersandAmpersand, TokenType_PipePipe,
    TokenType_ShiftLeft, TokenType_ShiftRight,
    
    TokenType_Percent,
    TokenType_PlusPlus, TokenType_MinusMinus,
    TokenType_Backslash, TokenType_Hat, TokenType_Equal,
    TokenType_PlusEqual, TokenType_MinusEqual, TokenType_StarEqual,
    TokenType_SlashEqual, TokenType_PercentEqual, TokenType_AmpersandEqual,
    TokenType_PipeEqual,
    TokenType_HatEqual, TokenType_TildeEqual,
    
    TokenType_OpenBrace, TokenType_OpenParenthesis, TokenType_OpenBracket,
    TokenType_CloseBrace, TokenType_CloseParenthesis, TokenType_CloseBracket,
    TokenType_Comma, TokenType_Dot, TokenType_Semicolon, TokenType_Colon,
    TokenType_Question, TokenType_Arrow, TokenType_ThinArrow,
    
    TokenType_Struct, TokenType_Enum, TokenType_Union, TokenType_FlagEnum,
    TokenType_Return, TokenType_Break, TokenType_Continue, TokenType_Import,
    TokenType_Null, TokenType_Nullptr, TokenType_Const,
    TokenType_If, TokenType_Else, TokenType_Do, TokenType_For, TokenType_While,
    TokenType_Switch, TokenType_Match, TokenType_Case, TokenType_Default,
    TokenType_True, TokenType_False, TokenType_Native,
    TokenType_Namespace, TokenType_Using,
    TokenType_Int, TokenType_Cstring, TokenType_Float, TokenType_Bool,
    TokenType_Double, TokenType_Char, TokenType_Short, TokenType_Long,
    TokenType_Void, TokenType_Uchar, TokenType_Ushort,
    TokenType_Uint, TokenType_Ulong,
    
    TokenType_Tag, TokenType_Sizeof, TokenType_Offsetof, TokenType_Cinclude,
    TokenType_Cinsert, TokenType_Operator, TokenType_Typedef,
    
    TokenType_TokenTypeCount
};

typedef struct __L_TokenTypeStringPair {
    L_TokenType type;
    string name;
} __L_TokenTypeStringPair;

typedef struct L_Lexer {
    const char* start;
    const char* current;
    u32 line;
} L_Lexer;

typedef struct L_Token {
    L_TokenType type;
    const char* start;
    u32 line, length;
} L_Token;

void L_Initialize(L_Lexer* lexer, string source);
L_Token L_LexToken(L_Lexer* lexer);
void L_PrintToken(L_Token token);

string L__get_string_from_type__(L_TokenType type);

#endif //LEXER_H
