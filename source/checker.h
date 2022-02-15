/* date = February 15th 2022 7:14 am */

#ifndef CHECKER_H
#define CHECKER_H

#include "defines.h"
#include "parser.h"
#include "mem.h"

// Uses TYPE(Id, Name)
#define BASIC_TYPES \
TYPE(Invalid, str_lit("Invalid")) \
TYPE(Integer, str_lit("Integer"))

typedef u32 C_BasicType;
enum {
    BasicType_Invalid,
    BasicType_Integer
};

typedef struct C_Checker {
    b8 errored;
    u8 error_count;
} C_Checker;

void C_CheckAst(C_Checker* checker, AstNode* node);

#endif //CHECKER_H
