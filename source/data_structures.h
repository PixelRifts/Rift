/* date = October 26th 2021 1:32 pm */

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#define TABLE_MAX_LOAD 0.75
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_CAPACITY_BIGGER(capacity) ((capacity) < 32 ? 32 : (capacity) * 2)

#include "defines.h"
#include "str.h"
#include "types.h"

typedef struct var_entry_key {
    string name;
    u32 depth;
} var_entry_key;

typedef struct var_table_entry {
    var_entry_key key;
    P_ValueType value;
} var_table_entry;

typedef struct var_hash_table {
    u32 count;
    u32 capacity;
    var_table_entry* entries;
} var_hash_table;

void var_hash_table_init(var_hash_table* table);
void var_hash_table_free(var_hash_table* table);
b8   var_hash_table_get(var_hash_table* table, var_entry_key key, P_ValueType* value);
b8   var_hash_table_set(var_hash_table* table, var_entry_key key, P_ValueType  value);
b8   var_hash_table_del(var_hash_table* table, var_entry_key key);
void var_hash_table_add_all(var_hash_table* from, var_hash_table* to);


typedef struct func_entry_key {
    string name;
    i32 depth;
} func_entry_key;

// Values are in a linked list. this feels evil
typedef struct func_entry_val {
    P_ValueType value;
    string mangled_name;
    value_type_list param_types;
    b8 is_native;
    b8 is_varargs;
    
    struct func_entry_val* next;
} func_entry_val;

typedef struct func_table_entry {
    func_entry_key  key;
    func_entry_val* value;
} func_table_entry;

typedef struct func_hash_table {
    u32 count;
    u32 capacity;
    func_table_entry* entries;
} func_hash_table;

void func_hash_table_init(func_hash_table* table);
void func_hash_table_free(func_hash_table* table);
b8   func_hash_table_get_absp(func_hash_table* table, func_entry_key key, value_type_list* param_types, func_entry_val** value, u32* subset_match, b8 absolute_check, b8 no_varargs);
b8   func_hash_table_get(func_hash_table* table, func_entry_key key, value_type_list* param_types, func_entry_val** value, u32* subset_match, b8 absolute_check);
b8   func_hash_table_has_name(func_hash_table* table, func_entry_key key);
b8   func_hash_table_set(func_hash_table* table, func_entry_key key, func_entry_val* value);
b8   func_hash_table_del(func_hash_table* table, func_entry_key key, string mangled_needle);
b8   func_hash_table_del_full(func_hash_table* table, func_entry_key key);
void func_hash_table_add_all(func_hash_table* from, func_hash_table* to);


typedef u32 P_ContainerType;
enum {
    ContainerType_Enum, ContainerType_Struct,
};

typedef struct P_Container {
    P_ContainerType type;
    string name;
    u32 depth;
    u32 member_count;
    value_type_list member_types;
    string_list member_names;
    b8 is_native;
} P_Container;

typedef struct type_array {
    u32 capacity;
    u32 count;
    P_Container* elements;
} type_array;

void type_array_init(type_array* array);
void type_array_free(type_array* array);
void type_array_add(type_array* array, P_Container structure);


typedef struct type_mod_array {
    u32 capacity;
    u32 count;
    P_ValueTypeMod* elements;
} type_mod_array;

void type_mod_array_add(M_Arena* arena, type_mod_array* array, P_ValueTypeMod mod);

typedef struct expr_array {
    u32 capacity;
    u32 count;
    struct P_Expr** elements;
} expr_array;

void expr_array_add(M_Arena* arena, expr_array* array, struct P_Expr* mod);

struct P_Namespace;

typedef struct namespace_array {
    u32 capacity;
    u32 count;
    struct P_Namespace** elements;
} namespace_array;

void namespace_array_add(M_Arena* arena, namespace_array* array, struct P_Namespace* mod);


#endif //DATA_STRUCTURES_H
