/* date = February 13th 2022 8:35 pm */

#ifndef PARSER_DATA_H
#define PARSER_DATA_H

typedef u32 Prec;
enum Prec {
    Prec_Invalid,
    Prec_LogOr,
    Prec_LogAnd,
    Prec_Equality,
    Prec_Compare,
    Prec_Term,
    Prec_Factor,
    Prec_Call,
    Prec_None,
};

u32 infix_expr_precs[] = {
    [TokenType_OpenParenthesis] = Prec_Call,
    
    [TokenType_Star] = Prec_Factor,
    [TokenType_Slash] = Prec_Factor,
    
    [TokenType_Plus] = Prec_Term,
    [TokenType_Minus] = Prec_Term,
    
    [TokenType_EqualEqual] = Prec_Equality,
    [TokenType_BangEqual] = Prec_Equality,
    
    [TokenType_Less] = Prec_Compare,
    [TokenType_LessEqual] = Prec_Compare,
    [TokenType_Greater] = Prec_Compare,
    [TokenType_GreaterEqual] = Prec_Compare,
    
    [TokenType_AmpersandAmpersand] = Prec_LogAnd,
    [TokenType_PipePipe] = Prec_LogOr,
    
    [TokenType_TokenTypeCount] = Prec_Invalid,
};

#endif //PARSER_DATA_H
