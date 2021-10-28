/* date = October 26th 2021 1:32 pm */

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#define TABLE_MAX_LOAD 0.75
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

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
    string_list param_types;
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
// Part decides how many params at the end don't get checked for varargs
b8   func_hash_table_get(func_hash_table* table, func_entry_key key, string_list param_types, func_entry_val** value, u32* subset_match);
b8   func_hash_table_set(func_hash_table* table, func_entry_key key, func_entry_val* value);
b8   func_hash_table_del(func_hash_table* table, func_entry_key key);
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
    string_list member_types;
    string_list member_names;
} P_Container;

typedef struct type_array {
    u32 capacity;
    u32 count;
    P_Container* elements;
} type_array;

void type_array_init(type_array* array);
void type_array_free(type_array* array);
void type_array_add(type_array* array, P_Container structure);

#endif //DATA_STRUCTURES_H
