/* date = October 27th 2021 7:29 pm */

#ifndef TYPES_H
#define TYPES_H

#include "defines.h"
#include "str.h"

struct P_Expr;

typedef u32 P_ValueTypeModType;
enum P_ValueTypeModType {
    ValueTypeModType_Pointer,
    ValueTypeModType_Array
};

typedef struct P_ValueTypeMod {
    P_ValueTypeModType type;
    // Union bcoz maybe ill need some other thingy here. highly unlikely but ya never know
    union {
        struct P_Expr* array_expr;
    } op;
} P_ValueTypeMod;

typedef struct P_ValueType {
    string base_type;
    string full_type;
    P_ValueTypeMod* mods;
    u32 mod_ct;
} P_ValueType;

typedef struct value_type_list_node {
    P_ValueType type;
    struct value_type_list_node* next;
} value_type_list_node;

typedef struct value_type_list {
    value_type_list_node* first;
    value_type_list_node* last;
    
    i32 node_count;
} value_type_list;

#define value_type_abs(s) (P_ValueType) { .base_type = str_lit(s), .full_type = str_lit(s) }
#define value_type_from_str(s) (P_ValueType) { .base_type = (string) { .str = s.str, .size = s.size }, .full_type = (string) { .str = s.str, .size = s.size } }

void value_type_list_push_node(value_type_list* list, value_type_list_node* node);
void value_type_list_push(M_Arena* arena, value_type_list* list, P_ValueType type);

static const P_ValueType ValueType_Invalid = { .base_type = str_lit("INVALID"), .mods = nullptr };

static const P_ValueType ValueType_Integer = value_type_abs("int");
static const P_ValueType ValueType_Long = value_type_abs("long");
static const P_ValueType ValueType_Float = value_type_abs("float");
static const P_ValueType ValueType_Double = value_type_abs("double");

static const P_ValueType ValueType_String = value_type_abs("string");
static const P_ValueType ValueType_Char = value_type_abs("char");
static const P_ValueType ValueType_Bool = value_type_abs("bool");
static const P_ValueType ValueType_Void = value_type_abs("void");
static const P_ValueType ValueType_Tombstone = value_type_abs("tombstone");

b8 type_check(P_ValueType a, P_ValueType expected);
b8 node_type_check(value_type_list_node* a, value_type_list_node* expected);
b8 value_type_list_equals(value_type_list* a, value_type_list* b);

#endif //TYPES_H
