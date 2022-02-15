/* date = February 15th 2022 2:26 pm */

#ifndef CHECKER_DATA_H
#define CHECKER_DATA_H

typedef struct C_BasicTypePair {
    C_BasicType a;
    C_BasicType b;
} C_BasicTypePair;

typedef struct C_OperatorBinding {
    C_BasicTypePair* pairs;
    u32 pairs_count;
} C_OperatorBinding;



const u32 operator_term_length = 1;
C_BasicTypePair operator_term[] = {
    { BasicType_Integer, BasicType_Integer }
};

const u32 operator_factor_length = 1;
C_BasicTypePair operator_factor[] = {
    { BasicType_Integer, BasicType_Integer }
};

#define associate(token, name) [token] = { operator_##name, operator_##name##_length }
C_OperatorBinding binary_operator_bindings[] = {
    associate(TokenType_Plus, term),
    associate(TokenType_Minus, term),
    associate(TokenType_Star, factor),
    associate(TokenType_Slash, factor),
    
    [TokenType_TokenTypeCount] = { nullptr, 0 },
};
#undef associate

#endif //CHECKER_DATA_H
