/* date = February 15th 2022 2:26 pm */

#ifndef CHECKER_DATA_H
#define CHECKER_DATA_H

//~ Unary Operators

typedef struct C_UnaryOpBinding {
    C_BasicType* elems;
    u32 count;
} C_UnaryOpBinding;

const u32 uoperator_number_length = 1;
C_BasicType uoperator_number[] = {
    BasicType_Integer,
};

#define associate(token, name) [token] = { uoperator_##name, uoperator_##name##_length }
C_UnaryOpBinding unary_operator_bindings[] = {
    associate(TokenType_Plus, number),
    associate(TokenType_Minus, number),
    associate(TokenType_Tilde, number),
    
    [TokenType_TokenTypeCount] = { nullptr, 0 },
};
#undef associate


//~ Binary Operators

typedef struct C_BasicTypePair {
    C_BasicType a;
    C_BasicType b;
} C_BasicTypePair;

typedef struct C_BinaryOpBinding {
    C_BasicTypePair* pairs;
    u32 pairs_count;
} C_BinaryOpBinding;

const u32 operator_term_length = 1;
C_BasicTypePair operator_term[] = {
    { BasicType_Integer, BasicType_Integer }
};

const u32 operator_factor_length = 1;
C_BasicTypePair operator_factor[] = {
    { BasicType_Integer, BasicType_Integer }
};

#define associate(token, name) [token] = { operator_##name, operator_##name##_length }
C_BinaryOpBinding binary_operator_bindings[] = {
    associate(TokenType_Plus, term),
    associate(TokenType_Minus, term),
    associate(TokenType_Star, factor),
    associate(TokenType_Slash, factor),
    
    [TokenType_TokenTypeCount] = { nullptr, 0 },
};
#undef associate

#endif //CHECKER_DATA_H
