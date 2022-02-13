/* date = February 13th 2022 8:35 pm */

#ifndef PARSER_DATA_H
#define PARSER_DATA_H

typedef u32 Prec;
enum Prec {
    Prec_Term,
    Prec_Factor,
    Prec_None,
};

u32 infix_expr_precs[] = {
    [TokenType_Plus] = Prec_Term,
    [TokenType_Minus] = Prec_Term,
    [TokenType_Star] = Prec_Factor,
    [TokenType_Slash] = Prec_Factor,
    [TokenType_TokenTypeCount] = 0,
};

#endif //PARSER_DATA_H
