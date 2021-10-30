/* date = October 9th 2021 7:43 pm */

#ifndef OPERATOR_BINDINGS_H
#define OPERATOR_BINDINGS_H

typedef struct P_BinaryOpPair {
    P_ValueTypeCollection left;
    P_ValueTypeCollection right;
} P_BinaryOpPair;

P_ValueTypeCollection list_operator_arithmetic[] = {
    ValueTypeCollection_Number
};

P_ValueTypeCollection list_operator_bin[] = {
    ValueTypeCollection_Number
};

P_ValueTypeCollection list_operator_logical[] = {
    ValueTypeCollection_Bool
};

P_BinaryOpPair pairs_operator_arithmetic_term[] = {
    { .left = ValueTypeCollection_Number, .right = ValueTypeCollection_Number },
    { .left = ValueTypeCollection_Pointer, .right = ValueTypeCollection_Number },
    { .left = ValueTypeCollection_Number, .right = ValueTypeCollection_Pointer },
};

P_BinaryOpPair pairs_operator_arithmetic_factor[] = {
    { .left = ValueTypeCollection_Number, .right = ValueTypeCollection_Number },
};

P_BinaryOpPair pairs_operator_bin[] = {
    { .left = ValueTypeCollection_WholeNumber, .right = ValueTypeCollection_WholeNumber },
};

P_BinaryOpPair pairs_operator_equality[] = {
    { .left = ValueTypeCollection_Number, .right = ValueTypeCollection_Number },
    { .left = ValueTypeCollection_String, .right = ValueTypeCollection_String },
    { .left = ValueTypeCollection_Char, .right = ValueTypeCollection_Char },
    { .left = ValueTypeCollection_Bool, .right = ValueTypeCollection_Bool },
};

P_BinaryOpPair pairs_operator_cmp[] = {
    { .left = ValueTypeCollection_Number, .right = ValueTypeCollection_Number },
    { .left = ValueTypeCollection_Char, .right = ValueTypeCollection_Char },
    { .left = ValueTypeCollection_Bool, .right = ValueTypeCollection_Bool },
};

P_BinaryOpPair pairs_operator_logical[] = {
    { .left = ValueTypeCollection_Bool, .right = ValueTypeCollection_Bool },
};

#endif //OPERATOR_BINDINGS_H
