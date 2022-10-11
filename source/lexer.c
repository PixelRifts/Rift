#include "lexer.h"
#include <string.h>
#include <stdio.h>

string L_GetTypeName(L_TokenType type) {
    switch (type) {
        case TokenType_Error: return str_lit("Error");
        case TokenType_EOF: return str_lit("EOF");
        case TokenType_Ident: return str_lit("Identifier");
        case TokenType_CstringLit: return str_lit("Null-terminated String Literal");
        case TokenType_CharLit: return str_lit("Character Literal");
        case TokenType_IntLit: return str_lit("Integer Literal");
        case TokenType_FloatLit: return str_lit("Float Literal");
        case TokenType_DoubleLit: return str_lit("Double Literal");
        case TokenType_LongLit: return str_lit("Long Literal");
        case TokenType_Plus: return str_lit("+");
        case TokenType_Minus: return str_lit("-");
        case TokenType_Star: return str_lit("*");
        case TokenType_Slash: return str_lit("/");
        case TokenType_Percent: return str_lit("%");
        case TokenType_PlusPlus: return str_lit("++");
        case TokenType_MinusMinus: return str_lit("--");
        case TokenType_Backslash: return str_lit("\\");
        case TokenType_Hat: return str_lit("^");
        case TokenType_Ampersand: return str_lit("&");
        case TokenType_Pipe: return str_lit("|");
        case TokenType_Tilde: return str_lit("~");
        case TokenType_Bang: return str_lit("!");
        case TokenType_Equal: return str_lit("=");
        case TokenType_PlusEqual: return str_lit("+=");
        case TokenType_MinusEqual: return str_lit("-=");
        case TokenType_StarEqual: return str_lit("*=");
        case TokenType_SlashEqual: return str_lit("/=");
        case TokenType_PercentEqual: return str_lit("%=");
        case TokenType_AmpersandEqual: return str_lit("&=");
        case TokenType_PipeEqual: return str_lit("|=");
        case TokenType_HatEqual: return str_lit("^=");
        case TokenType_TildeEqual: return str_lit("~=");
        case TokenType_BangEqual: return str_lit("!=");
        case TokenType_EqualEqual: return str_lit("==");
        case TokenType_Less: return str_lit("<");
        case TokenType_Greater: return str_lit(">");
        case TokenType_LessEqual: return str_lit("<=");
        case TokenType_GreaterEqual: return str_lit(">=");
        case TokenType_Arrow: return str_lit("=>");
        case TokenType_AmpersandAmpersand: return str_lit("&&");
        case TokenType_PipePipe: return str_lit("||");
        case TokenType_OpenBrace: return str_lit("{");
        case TokenType_OpenParenthesis: return str_lit("(");
        case TokenType_OpenBracket: return str_lit("[");
        case TokenType_CloseBrace: return str_lit("}");
        case TokenType_CloseParenthesis: return str_lit(")");
        case TokenType_CloseBracket: return str_lit("]");
        case TokenType_Dot: return str_lit(".");
        case TokenType_Semicolon: return str_lit(";");
        case TokenType_Colon: return str_lit(":");
        case TokenType_Question: return str_lit("?");
        case TokenType_Return: return str_lit("Return");
        case TokenType_Import: return str_lit("Import");
        case TokenType_Struct: return str_lit("Struct");
        case TokenType_Union: return str_lit("Union");
        case TokenType_Enum: return str_lit("Enum");
        case TokenType_FlagEnum: return str_lit("FlagEnum");
        case TokenType_Null: return str_lit("Null");
        case TokenType_Nullptr: return str_lit("Nullptr");
        case TokenType_Break: return str_lit("Break");
        case TokenType_Continue: return str_lit("Continue");
        case TokenType_Const: return str_lit("Const");
        case TokenType_Switch: return str_lit("Switch");
        case TokenType_Match: return str_lit("Match");
        case TokenType_Case: return str_lit("Case");
        case TokenType_Default: return str_lit("Default");
        case TokenType_If: return str_lit("If");
        case TokenType_Else: return str_lit("Else");
        case TokenType_Do: return str_lit("Do");
        case TokenType_For: return str_lit("For");
        case TokenType_While: return str_lit("While");
        case TokenType_True: return str_lit("True");
        case TokenType_False: return str_lit("False");
        case TokenType_Void: return str_lit("Void");
        case TokenType_Int: return str_lit("Int");
        case TokenType_Short: return str_lit("Short");
        case TokenType_Bool: return str_lit("Bool");
        case TokenType_Double: return str_lit("Double");
        case TokenType_Char: return str_lit("Char");
        case TokenType_Long: return str_lit("Long");
        case TokenType_Uchar: return str_lit("Uchar");
        case TokenType_Ushort: return str_lit("Ushort");
        case TokenType_Uint: return str_lit("Uint");
        case TokenType_Ulong: return str_lit("Ulong");
        case TokenType_Cstring: return str_lit("Null-Terminated String");
        case TokenType_Native: return str_lit("Native");
        case TokenType_Namespace: return str_lit("Namespace");
        case TokenType_Using: return str_lit("Using");
        case TokenType_Cinclude: return str_lit("Cinclude");
        case TokenType_Cinsert: return str_lit("Cinsert");
        case TokenType_Operator: return str_lit("Operator");
        case TokenType_Typedef: return str_lit("Typedef");
        case TokenType_Print: return str_lit("Print");
    }
    return str_lit("unreachable");
}

static b8 is_digit(i8 c) { return c >= '0' && c <= '9'; }
static b8 is_whitespace(i8 c) { return c == ' ' || c == '\r' || c == '\n' || c == '\t'; }
static b8 is_alpha(i8 c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

static inline b8 L_Bound(L_Lexer* lexer) { return *lexer->current == '\0'; }

static i8 L_Advance(L_Lexer* lexer) {
    lexer->current++;
    lexer->column++;
    return lexer->current[-1];
}

static b8 L_Match(L_Lexer* lexer, i8 expected) {
    if (L_Bound(lexer)) return false;
    if (*lexer->current != expected) return false;
    L_Advance(lexer);
    return true;
}

static L_Token L_MakeToken(L_Lexer* lexer, L_TokenType type) {
    return (L_Token) {
        .type = type,
        .lexeme = (string) { .str = (u8*)lexer->start, .size = (u32) (lexer->current - lexer->start) },
        .line = lexer->line,
        .column = lexer->column,
    };
}

static inline L_Token L_ErrorToken(L_Lexer* lexer, string msg) {
    return (L_Token) {
        .type = TokenType_Error,
        .lexeme = msg,
        .line = lexer->line,
        .column = lexer->column,
    };
}

static L_Token L_DoubleHandle(L_Lexer* lexer, i8 expected, L_TokenType yes, L_TokenType no) {
    if (L_Match(lexer, expected)) return L_MakeToken(lexer, yes);
    return L_MakeToken(lexer, no);
}

static L_Token L_TripleHandle(L_Lexer* lexer, i8 expected1, L_TokenType yes1, i8 expected2, L_TokenType yes2, L_TokenType no) {
    if (L_Match(lexer, expected1)) return L_MakeToken(lexer, yes1);
    else if (L_Match(lexer, expected2)) return L_MakeToken(lexer, yes2);
    return L_MakeToken(lexer, no);
}

static L_Token L_QuadHandle(L_Lexer* lexer, i8 expected1, L_TokenType yes1, i8 expected2, L_TokenType yes2, i8 expected3, L_TokenType yes3, L_TokenType no) {
    if (L_Match(lexer, expected1)) return L_MakeToken(lexer, yes1);
    else if (L_Match(lexer, expected2)) return L_MakeToken(lexer, yes2);
    else if (L_Match(lexer, expected3)) return L_MakeToken(lexer, yes3);
    return L_MakeToken(lexer, no);
}

static i8 L_Peek(L_Lexer* lexer) { return *lexer->current; }

static i8 L_PeekNext(L_Lexer* lexer) {
    if (L_Bound(lexer)) return '\0';
    return lexer->current[1];
}

static void L_SkipWhitespace(L_Lexer* lexer) {
    while (true) {
        i8 c = L_Peek(lexer);
        switch (c) {
            case ' ': case '\r': case '\t': L_Advance(lexer); break;
            case '\n': lexer->line++; lexer->column = 0; L_Advance(lexer); break;
            default: return;
        }
    }
}

static L_Token L_String(L_Lexer* lexer) {
    while (L_Peek(lexer) != '"') {
        if (L_Peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 0;
        }
        if (L_Bound(lexer)) return L_ErrorToken(lexer, str_lit("Unterminated String literal"));
        L_Advance(lexer);
    }
    L_Advance(lexer); // Closing quote
    return L_MakeToken(lexer, TokenType_CstringLit);
}

static L_Token L_Char(L_Lexer* lexer) {
    if (L_Peek(lexer) == '\\') { L_Advance(lexer); }
    if (L_PeekNext(lexer) == '\'') {
        L_Advance(lexer);
        L_Advance(lexer);
    } else {
        return L_ErrorToken(lexer, str_lit("Only one character permitted in character literal"));
    }
    return L_MakeToken(lexer, TokenType_CharLit);
}

static L_Token L_Number(L_Lexer* lexer) {
    L_TokenType type = TokenType_IntLit;
    while (is_digit(L_Peek(lexer))) L_Advance(lexer);
    
    if (L_Peek(lexer) == '.') {
        L_Advance(lexer); // Consume .
        
        while (is_digit(L_Peek(lexer))) L_Advance(lexer);
        
        if (L_Peek(lexer) == 'f' || L_Peek(lexer) == 'F') {
            L_Advance(lexer);
            type = TokenType_FloatLit;
        } else if (L_Peek(lexer) == 'd' || L_Peek(lexer) == 'D') {
            L_Advance(lexer);
            type = TokenType_DoubleLit;
        } else if (!is_alpha(L_Peek(lexer))) {
            type = TokenType_DoubleLit;
        } else {
            return L_ErrorToken(lexer, str_lit("Unrecognised number suffix"));
        }
    } else if (L_Peek(lexer) == 'l' || L_Peek(lexer) == 'L') {
        L_Advance(lexer);
        type = TokenType_LongLit;
    }
    
    return L_MakeToken(lexer, type);
}

static L_TokenType L_MatchType(L_Lexer* lexer, u32 start, string needle, L_TokenType yes) {
    if (lexer->current - lexer->start == start + needle.size && memcmp(lexer->start + start, needle.str, needle.size) == 0) {
        return yes;
    }
    return TokenType_Ident;
}

static L_TokenType L_IdentifierType(L_Lexer* lexer) {
    switch (lexer->start[0]) {
        case '#': return L_MatchType(lexer, 1, str_lit("native"), TokenType_Native);
        
        case 'r': return L_MatchType(lexer, 1, str_lit("eturn"), TokenType_Return);
        case 'm': return L_MatchType(lexer, 1, str_lit("atch"), TokenType_Match);
        case 'w': return L_MatchType(lexer, 1, str_lit("hile"), TokenType_While);
        case 'l': return L_MatchType(lexer, 1, str_lit("ong"), TokenType_Long);
        case 'v': return L_MatchType(lexer, 1, str_lit("oid"), TokenType_Void);
        case 'p': return L_MatchType(lexer, 1, str_lit("rint"), TokenType_Print);
		
        case 't': {
            switch (lexer->start[1]) {
                case 'r': return L_MatchType(lexer, 2, str_lit("ue"), TokenType_True);
                case 'y': return L_MatchType(lexer, 2, str_lit("pedef"), TokenType_Typedef);
            }
            return TokenType_Ident;
        }
        
        case 'u': {
            switch (lexer->start[1]) {
                case 's': {
                    switch (lexer->start[2]) {
                        case 'i': return L_MatchType(lexer, 3, str_lit("ng"), TokenType_Using);
                        case 'h': return L_MatchType(lexer, 3, str_lit("ort"), TokenType_Ushort);
                    }
                }
                case 'c': return L_MatchType(lexer, 2, str_lit("har"), TokenType_Uchar);
                case 'i': return L_MatchType(lexer, 2, str_lit("nt"),  TokenType_Uint);
                case 'l': return L_MatchType(lexer, 2, str_lit("ong"), TokenType_Ulong);
                case 'n': return L_MatchType(lexer, 2, str_lit("ion"), TokenType_Union);
            }
            return TokenType_Ident;
        }
        
        case 'o': {
            switch (lexer->start[1]) {
                case 'f': return L_MatchType(lexer, 2, str_lit("fsetof"), TokenType_Offsetof);
                case 'p': return L_MatchType(lexer, 2, str_lit("erator"), TokenType_Operator);
            }
            return TokenType_Ident;
        }
        
        case 'b': {
            switch (lexer->start[1]) {
                case 'o': return L_MatchType(lexer, 2, str_lit("ol"), TokenType_Bool);
                case 'r': return L_MatchType(lexer, 2, str_lit("eak"), TokenType_Break);
            }
            return TokenType_Ident;
        }
        
        case 'c': {
            switch (lexer->start[1]) {
                case 's': return L_MatchType(lexer, 2, str_lit("tring"), TokenType_Cstring);
                case 'h': return L_MatchType(lexer, 2, str_lit("ar"), TokenType_Char);
                case 'o': {
                    if (L_MatchType(lexer, 2, str_lit("ntinue"), TokenType_Continue) == TokenType_Continue) {
                        return TokenType_Continue;
                    } else return L_MatchType(lexer, 2, str_lit("nst"), TokenType_Const);
                }
                case 'a': return L_MatchType(lexer, 2, str_lit("se"), TokenType_Case);
                case 'i': {
                    if (L_MatchType(lexer, 2, str_lit("nclude"), TokenType_Cinclude) == TokenType_Cinclude) {
                        return TokenType_Cinclude;
                    } else return L_MatchType(lexer, 2, str_lit("nsert"), TokenType_Cinsert);
                }
            }
            return TokenType_Ident;
        }
        
        case 's': {
            switch (lexer->start[1]) {
                case 't': return L_MatchType(lexer, 2, str_lit("ruct"), TokenType_Struct);
                case 'w': return L_MatchType(lexer, 2, str_lit("itch"), TokenType_Switch);
                case 'i': return L_MatchType(lexer, 2, str_lit("zeof"), TokenType_Sizeof);
                case 'h': return L_MatchType(lexer, 2, str_lit("ort"),  TokenType_Short);
            }
            return TokenType_Ident;
        }
        
        case 'e': {
            switch (lexer->start[1]) {
                case 'n': return L_MatchType(lexer, 2, str_lit("um"), TokenType_Enum);
                case 'l': return L_MatchType(lexer, 2, str_lit("se"), TokenType_Else);
            }
            return TokenType_Ident;
        }
        
        case 'n': {
            switch (lexer->start[1]) {
                case 'u': {
                    if (L_MatchType(lexer, 2, str_lit("llptr"), TokenType_Nullptr) == TokenType_Nullptr) {
                        return TokenType_Nullptr;
                    } else return L_MatchType(lexer, 2, str_lit("ll"), TokenType_Null);
                }
                case 'a': {
                    return L_MatchType(lexer, 2, str_lit("mespace"), TokenType_Namespace);
                }
            }
            return TokenType_Ident;
        }
        
        case 'i': {
            switch (lexer->start[1]) {
                case 'm': return L_MatchType(lexer, 2, str_lit("port"), TokenType_Import);
                case 'n': return L_MatchType(lexer, 2, str_lit("t"), TokenType_Int);
                default: return L_MatchType(lexer, 1, str_lit("f"), TokenType_If);
            }
            return TokenType_Ident;
        }
        
        case 'd': {
            switch (lexer->start[1]) {
                case 'o': {
                    if (L_MatchType(lexer, 2, str_lit("uble"), TokenType_Double) == TokenType_Double) {
                        return TokenType_Double;
                    } else return L_MatchType(lexer, 1, str_lit("o"), TokenType_Do);
                }
                case 'e': return L_MatchType(lexer, 2, str_lit("fault"), TokenType_Default);
            }
            return TokenType_Ident;
        }
        
        case 'f': {
            switch (lexer->start[1]) {
                case 'o': return L_MatchType(lexer, 2, str_lit("r"), TokenType_For);
                case 'l': {
                    switch (lexer->start[2]) {
                        case 'o': return L_MatchType(lexer, 3, str_lit("at"), TokenType_Float);
                        case 'a': return L_MatchType(lexer, 3, str_lit("genum"), TokenType_FlagEnum);
                    }
                }
                case 'a': return L_MatchType(lexer, 2, str_lit("lse"), TokenType_False);
                case 'u': return L_MatchType(lexer, 2, str_lit("nc"), TokenType_Func);
            }
            return TokenType_Ident;
        }
	}
    return TokenType_Ident;
}

static L_Token L_Identifier(L_Lexer* lexer) {
    while (is_alpha(L_Peek(lexer)) || is_digit(L_Peek(lexer))) {
        L_Advance(lexer);
    }
    
    return L_MakeToken(lexer, L_IdentifierType(lexer));
}

static L_Token L_Tag(L_Lexer* lexer) {
    while (is_alpha(L_Peek(lexer)) || is_digit(L_Peek(lexer))) {
        L_Advance(lexer);
    }
    
    return L_MakeToken(lexer, TokenType_Tag);
}

void L_Init(L_Lexer* lexer, string source) {
    lexer->start = (const char*) source.str;
    lexer->current = (const char*) source.str;
    lexer->line = 1;
    lexer->column = 0;
}

L_Token L_LexToken(L_Lexer* lexer) {
    L_SkipWhitespace(lexer);
    lexer->start = lexer->current;
    
    if (L_Bound(lexer)) return L_MakeToken(lexer, TokenType_EOF);
    i8 c = L_Advance(lexer);
    
    switch (c) {
        case '\\': return L_MakeToken(lexer, TokenType_Backslash);
        case '(':  return L_MakeToken(lexer, TokenType_OpenParenthesis);
        case '[':  return L_MakeToken(lexer, TokenType_OpenBracket);
        case '{':  return L_MakeToken(lexer, TokenType_OpenBrace);
        case ')':  return L_MakeToken(lexer, TokenType_CloseParenthesis);
        case ']':  return L_MakeToken(lexer, TokenType_CloseBracket);
        case '}':  return L_MakeToken(lexer, TokenType_CloseBrace);
        case ',':  return L_MakeToken(lexer, TokenType_Comma);
        case ';':  return L_MakeToken(lexer, TokenType_Semicolon);
        case ':':  return L_MakeToken(lexer, TokenType_Colon);
        case '?':  return L_MakeToken(lexer, TokenType_Question);
        
        case '=':  return L_TripleHandle(lexer, '=', TokenType_EqualEqual, '>', TokenType_Arrow, TokenType_Equal);
        case '!':  return L_DoubleHandle(lexer, '=', TokenType_BangEqual, TokenType_Bang);
        case '|':  return L_TripleHandle(lexer, '=', TokenType_PipeEqual, '|', TokenType_PipePipe, TokenType_Pipe);
        case '~':  return L_DoubleHandle(lexer, '=', TokenType_TildeEqual, TokenType_Tilde);
        case '^':  return L_DoubleHandle(lexer, '=', TokenType_HatEqual, TokenType_Hat);
        case '&':  return L_TripleHandle(lexer, '=', TokenType_AmpersandEqual, '&', TokenType_AmpersandAmpersand, TokenType_Ampersand);
        case '+': return L_TripleHandle(lexer, '=', TokenType_PlusEqual, '+', TokenType_PlusPlus, TokenType_Plus);
        case '-': return L_QuadHandle(lexer, '=', TokenType_MinusEqual, '-', TokenType_MinusMinus, '>', TokenType_ThinArrow, TokenType_Minus);
        case '*':  return L_DoubleHandle(lexer, '=', TokenType_StarEqual, TokenType_Star);
        case '%':  return L_DoubleHandle(lexer, '=', TokenType_PercentEqual, TokenType_Percent);
        case '<':  return L_TripleHandle(lexer, '=', TokenType_LessEqual, '<', TokenType_ShiftLeft, TokenType_Less);
        case '>':  return L_TripleHandle(lexer, '=', TokenType_GreaterEqual, '>', TokenType_ShiftRight, TokenType_Greater);
        
        case '/':  {
            if (L_Peek(lexer) == '/') {
                while (L_Peek(lexer) != '\n' && !L_Bound(lexer))
                    L_Advance(lexer);
                return L_LexToken(lexer);
            } else if (L_Peek(lexer) == '*') {
                // TODO(voxel): Record the position of the /* here to be reported as error for unterminated comment block
                L_Advance(lexer);
                u32 depth = 1;
                while (depth != 0) {
                    L_Advance(lexer);
                    if (L_Peek(lexer) == '\n') {
                        lexer->line++;
                        lexer->column = 0;
                    }
                    
                    if (L_Bound(lexer)) return L_ErrorToken(lexer, str_lit("Unterminated Comment Block\n"));
                    if (L_Peek(lexer) == '*' && L_PeekNext(lexer) == '/') {
                        L_Advance(lexer);
                        L_Advance(lexer);
                        depth--;
                    } else if (L_Peek(lexer) == '/' && L_PeekNext(lexer) == '*') {
                        L_Advance(lexer);
                        L_Advance(lexer);
                        depth++;
                    }
                }
                return L_LexToken(lexer);
            } else {
                return L_DoubleHandle(lexer, '=', TokenType_SlashEqual, TokenType_Slash);
            }
        }
        case '.':  {
            if (L_Peek(lexer) == '.' && L_PeekNext(lexer) == '.') {
                L_Advance(lexer); L_Advance(lexer);
                return L_MakeToken(lexer, TokenType_Ellipses);
            }
            return L_MakeToken(lexer, TokenType_Dot);
        }
        
        case '"':  return L_String(lexer);
        case '\'': return L_Char(lexer);
        
        // TODO(voxel): Hex Bin Oct numbers
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': return L_Number(lexer);
        
        case '@': {
            i8 n = L_Advance(lexer);
            if (!is_alpha(n) && n != '!')
                return L_ErrorToken(lexer, str_lit("Expected identifier after @\n"));
            else
                return L_Tag(lexer);
        }
        
        default: {
            if (is_alpha(c) || c == '#')
                return L_Identifier(lexer);
        }
    }
    
    return L_ErrorToken(lexer, str_lit("Unexpected Character\n"));
}

void L_PrintToken(L_Token token) {
    printf("%.*s: %.*s\n", str_expand(token.lexeme), str_expand(L_GetTypeName(token.type)));
}
