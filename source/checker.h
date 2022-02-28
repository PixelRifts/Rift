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
TYPE(Integer, str_lit("Integer")) \
TYPE(Cstring, str_lit("Cstring")) \
TYPE(Count, str_lit("__Count"))

typedef u32 C_SymbolType;
enum {
    SymbolType_Invalid,
    SymbolType_Variable,
    SymbolType_Count
};

typedef u64 C_SymbolFlags;
enum {
    SymbolFlag_Whatever   = 0x1,
    SymbolFlag_Other      = 0x2,
    SymbolType_Otherother = 0x4,
};

typedef struct C_Symbol {
    C_SymbolType type;
    C_SymbolFlags flags;
    
    string name;
    
    union {
        P_Type* variable_type;
    };
} C_Symbol;

HashTable_Prototype(symbol, struct { string name; u32 depth; }, C_Symbol);

typedef struct C_Checker {
    b8 errored;
    u8 error_count;
    
    symbol_hash_table symbol_table;
} C_Checker;

P_Type C_AstTypeToCheckerType(AstNode* type);

void C_Init(C_Checker* checker);
void C_CheckAst(C_Checker* checker, AstNode* node);
void C_Free(C_Checker* checker);

#endif //CHECKER_H
