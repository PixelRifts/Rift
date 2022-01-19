/* date = October 27th 2021 7:29 pm */

#ifndef TYPES_H
#define TYPES_H

#include "defines.h"
#include "str.h"

struct P_Expr;
struct P_ValueType;
struct value_type_list;

typedef u32 P_ValueTypeModType;
enum P_ValueTypeModType {
    ValueTypeModType_Pointer,
    ValueTypeModType_Array,
    ValueTypeModType_Reference,
};

typedef struct P_ValueTypeMod {
    P_ValueTypeModType type;
    union {
        struct P_Expr* array_expr;
    } op;
} P_ValueTypeMod;

typedef u32 P_ValueTypeType;
enum {
    ValueTypeType_Basic,
    ValueTypeType_FuncPointer
};

struct P_Namespace;
typedef struct P_ValueType {
    P_ValueTypeType type;
    
    string base_type;
    string full_type;
    P_ValueTypeMod* mods;
    u32 mod_ct;
    union {
        struct {
            string no_nmspc_name;
            struct P_Namespace* nmspc;
        } basic;
        
        struct {
            struct P_ValueType* ret_type;
            struct value_type_list* func_param_types;
        } func_ptr;
    } op;
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

#define value_type_from_str(s) (P_ValueType) { .base_type = (string) { .str = s.str, .size = s.size }, .full_type = (string) { .str = s.str, .size = s.size } }
#define value_type_abs(s) (P_ValueType) { .base_type = (string) { .str = (u8*)s, .size = sizeof(s)-1 }, .full_type = (string) { .str = (u8*)s, .size = sizeof(s)-1 } }
#define value_type_abs_nc(s) { .base_type = (string) { .str = (u8*)s, .size = sizeof(s)-1 }, .full_type = (string) { .str = (u8*)s, .size = sizeof(s)-1 } }


extern const P_ValueType ValueType_Invalid;

extern const P_ValueType ValueType_Any;
extern const P_ValueType ValueType_Char;
extern const P_ValueType ValueType_Short;
extern const P_ValueType ValueType_Integer;
extern const P_ValueType ValueType_Long;
extern const P_ValueType ValueType_Float;
extern const P_ValueType ValueType_Double;
extern const P_ValueType ValueType_UnsignedChar;
extern const P_ValueType ValueType_UnsignedShort;
extern const P_ValueType ValueType_UnsignedInteger;
extern const P_ValueType ValueType_UnsignedLong;

extern const P_ValueType ValueType_Cstring;
extern const P_ValueType ValueType_Bool;
extern const P_ValueType ValueType_Void;
extern const P_ValueType ValueType_Tombstone;

// Mods get filled in in the init routine
extern P_ValueType ValueType_VoidPointer;


void value_type_list_push_node(value_type_list* list, value_type_list_node* node);
void value_type_list_push(M_Arena* arena, value_type_list* list, P_ValueType type);

void types_init(M_Arena* arena);
b8 type_check(P_ValueType a, P_ValueType expected);
b8 is_ptr(P_ValueType* a);
b8 is_array(P_ValueType* a);
b8 is_ref(P_ValueType* a);
b8 value_type_list_equals(value_type_list* a, value_type_list* b);

#endif //TYPES_H
