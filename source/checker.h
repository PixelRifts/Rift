/* date = February 15th 2022 7:14 am */

#ifndef CHECKER_H
#define CHECKER_H

#include "defines.h"
#include "parser.h"
#include "mem.h"
#include "ds.h"

// Uses TYPE(Id, Name)
#define BASIC_TYPES \
TYPE(Invalid, str_lit("Invalid")) \
TYPE(Integer, str_lit("Integer"))

typedef u32 C_BasicType;
enum {
    BasicType_Invalid,
    BasicType_Integer,
    BasicType_Cstring,
    BasicType_End
};

typedef struct C_Type {
    C_BasicType type;
    
    union {
        // Nothing for now
    };
} C_Type;

HashTable_Prototype(var, struct { string name; u32 depth; }, struct { C_BasicType type; });

typedef struct C_Checker {
    b8 errored;
    u8 error_count;
    
    var_hash_table var_table;
} C_Checker;

void C_Init(C_Checker* checker);
void C_CheckAst(C_Checker* checker, AstNode* node);
void C_Free(C_Checker* checker);

#endif //CHECKER_H
