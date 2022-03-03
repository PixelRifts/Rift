/* date = February 15th 2022 2:26 pm */

#ifndef CHECKER_DATA_H
#define CHECKER_DATA_H

const P_Type C_NullType    = { 0 };
const P_Type C_InvalidType = { BasicType_Invalid };
const P_Type C_VoidType    = { BasicType_Void };
const P_Type C_IntegerType = { BasicType_Integer };
const P_Type C_CstringType = { BasicType_Cstring };

#define C_IntTypeConst { BasicType_Integer }

//~ Unary Operators

typedef struct C_UnaryOpTypes {
    P_Type* a;
    P_Type* ret;
} C_UnaryOpTypes;

typedef struct C_UnaryOpBinding {
    C_UnaryOpTypes* elems;
    u32 count;
} C_UnaryOpBinding;

const u32 uoperator_number_length = 1;
C_UnaryOpTypes uoperator_number[] = {
    { &C_IntegerType, &C_IntegerType },
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

typedef struct C_BinaryOpTypes {
    P_Type* a;
    P_Type* b;
    P_Type* ret;
} C_BinaryOpTypes;

typedef struct C_BinaryOpBinding {
    C_BinaryOpTypes* pairs;
    u32 pairs_count;
} C_BinaryOpBinding;

const u32 operator_term_length = 1;
C_BinaryOpTypes operator_term[] = {
    { &C_IntegerType, &C_IntegerType, &C_IntegerType }
};

const u32 operator_factor_length = 1;
C_BinaryOpTypes operator_factor[] = {
    { &C_IntegerType, &C_IntegerType, &C_IntegerType }
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
