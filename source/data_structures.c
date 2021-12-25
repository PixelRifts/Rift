#include "data_structures.h"

static const func_entry_val tombstone = {
    .value = {
        .base_type = str_lit("tombstone"),
        .mods = nullptr
    },
    .is_native = false
};

static const var_entry_val tombstone_var = {
    .mangled_name = { .str = nullptr, .size = 0 },
    .type = {
        .base_type = str_lit("tombstone"),
        .mods = nullptr
    }
};

//~ Variable Hashtable
static u32 hash_var_key(var_entry_key k) {
    u32 hash = 2166136261u;
    for (u32 i = 0; i < k.name.size; i++) {
        hash ^= k.name.str[i];
        hash *= 16777619;
    }
    hash += k.depth;
    return hash;
}

static var_table_entry* find_var_entry(var_table_entry* entries, i32 cap, var_entry_key key) {
    u32 idx = hash_var_key(key) % cap;
    var_table_entry* tombstone_e = nullptr;
    
    while (true) {
        var_table_entry* entry = &entries[idx];
        if (entry->key.name.size == 0) {
            if (entry->value.type.base_type.size == 0)
                return tombstone_e != nullptr ? tombstone_e : entry;
            else
                if (tombstone_e == nullptr) tombstone_e = entry;
        } else if (entry->key.name.size == key.name.size)
            if (memcmp(entry->key.name.str, key.name.str, key.name.size) == 0 && entry->key.depth == key.depth) {
            return entry;
        }
        idx = (idx + 1) % cap;
    }
}

static void var_table_adjust_cap(var_hash_table* table, i32 cap) {
    var_table_entry* entries = malloc(sizeof(var_table_entry) * cap);
    memset(entries, 0, sizeof(var_table_entry) * cap);
    
    table->count = 0;
    for (u32 i = 0; i < table->capacity; i++) {
        var_table_entry* entry = &table->entries[i];
        if (entry->key.name.size == 0) continue;
        
        var_table_entry* dest = find_var_entry(entries, cap, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }
    
    free(table->entries);
    table->entries = entries;
    table->capacity = cap;
}

void var_hash_table_init(var_hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = nullptr;
}

void var_hash_table_free(var_hash_table* table) {
    free(table->entries);
    table->count = 0;
    table->capacity = 0;
    table->entries = nullptr;
}

b8 var_hash_table_get(var_hash_table* table, var_entry_key key, var_entry_val* value) {
    if (table->count == 0) return false;
    var_table_entry* entry = find_var_entry(table->entries, table->capacity, key);
    if (entry->key.name.size == 0) return false;
    *value = entry->value;
    return true;
}

b8 var_hash_table_set(var_hash_table* table, var_entry_key key, var_entry_val value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        i32 cap = GROW_CAPACITY(table->capacity);
        var_table_adjust_cap(table, cap);
    }
    
    var_table_entry* entry = find_var_entry(table->entries, table->capacity, key);
    b8 is_new_key = entry->key.name.size == 0;
    if (is_new_key && entry->value.type.base_type.size == 0)
        table->count++;
    
    entry->key = key;
    entry->value = value;
    return is_new_key;
}

b8 var_hash_table_del(var_hash_table* table, var_entry_key key) {
    if (table->count == 0) return false;
    var_table_entry* entry = find_var_entry(table->entries, table->capacity, key);
    if (entry->key.name.size == 0) return false;
    entry->key = (var_entry_key) {0};
    entry->value = tombstone_var;
    return true;
}

void var_hash_table_add_all(var_hash_table* from, var_hash_table* to) {
    for (u32 i = 0; i < from->capacity; i++) {
        var_table_entry* entry = &from->entries[i];
        if (entry->key.name.size != 0) {
            var_hash_table_set(to, entry->key, entry->value);
        }
    }
}

//~ Function hashtable
static u32 hash_func_key(func_entry_key k) {
    u32 hash = 2166136261u;
    for (u32 i = 0; i < k.name.size; i++) {
        hash ^= k.name.str[i];
        hash *= 16777619;
    }
    hash += k.depth;
    return hash;
}

static func_table_entry* find_func_entry(func_table_entry* entries, i32 cap, func_entry_key key) {
    u32 idx = hash_func_key(key) % cap;
    func_table_entry* tombstone_e = nullptr;
    
    while (true) {
        func_table_entry* entry = &entries[idx];
        if (entry->key.name.size == 0) {
            if (entry->value == nullptr)
                return tombstone_e != nullptr ? tombstone_e : entry;
            else
                if (tombstone_e == nullptr) tombstone_e = entry;
        } else if (str_eq(entry->key.name, key.name) && entry->key.depth == key.depth)
            return entry;
        idx = (idx + 1) % cap;
    }
}

static void func_table_adjust_cap(func_hash_table* table, i32 cap) {
    func_table_entry* entries = malloc(sizeof(func_table_entry) * cap);
    memset(entries, 0, sizeof(func_table_entry) * cap);
    
    table->count = 0;
    for (u32 i = 0; i < table->capacity; i++) {
        func_table_entry* entry = &table->entries[i];
        if (entry->key.name.size == 0) continue;
        
        func_table_entry* dest = find_func_entry(entries, cap, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }
    
    free(table->entries);
    table->entries = entries;
    table->capacity = cap;
}

void func_hash_table_init(func_hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = nullptr;
}

void func_hash_table_free(func_hash_table* table) {
    free(table->entries);
    table->count = 0;
    table->capacity = 0;
    table->entries = nullptr;
}



b8 func_hash_table_get(func_hash_table* table, func_entry_key key, value_type_list* param_types, func_entry_val** value, u32* subset_match, b8 absolute_check) {
    return func_hash_table_get_absp(table, key, param_types, value, subset_match, absolute_check, false);
}

b8 func_hash_table_get_absp(func_hash_table* table, func_entry_key key, value_type_list* param_types, func_entry_val** value, u32* subset_match, b8 absolute_check, b8 no_varargs) {
    if (table->count == 0) return false;
    func_table_entry* entry = find_func_entry(table->entries, table->capacity, key);
    if (entry->key.name.size == 0) return false;
    func_entry_val* c = entry->value;
    
    u32 ctr = 0;
    b8 found = false;
    while (c != nullptr) {
        // If it wants more parameters than currently provided, just continue;
        if (no_varargs) {
            if (c->param_types.node_count != param_types->node_count) {
                c = c->next;
                continue;
            }
        } else if (c->param_types.node_count - 1 > param_types->node_count) {
            c = c->next;
            continue;
        }
        
        ctr = 0;
        // Check string_list equals. (can be a subset, so not using string_list_equals)
        value_type_list_node* curr_test = c->param_types.first;
        value_type_list_node* curr = param_types->first;
        while (!(curr_test == nullptr || curr == nullptr)) {
            
            if (!str_eq(str_lit("..."), curr_test->type.full_type)) {
                if (absolute_check) {
                    if (!str_eq(curr->type.full_type, curr_test->type.full_type)) break;
                } else
                    if (!type_check(curr->type, curr_test->type)) break;
                ctr++;
                curr_test = curr_test->next;
                curr = curr->next;
            } else {
                curr = curr->next;
            }
            
        }
        
        // If the while loop exited normally, we found a type match
        if (curr_test == nullptr || curr == nullptr) {
            found = true;
            break;
        }
        c = c->next;
    }
    
    if (found) {
        *value = c;
        *subset_match = ctr;
    }
    return found;
}

b8 func_hash_table_has_name(func_hash_table* table, func_entry_key key) {
    if (table->count == 0) return false;
    func_table_entry* entry = find_func_entry(table->entries, table->capacity, key);
    return entry->key.name.size != 0;
}

b8 func_hash_table_set(func_hash_table* table, func_entry_key key, func_entry_val* value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        i32 cap = GROW_CAPACITY(table->capacity);
        func_table_adjust_cap(table, cap);
    }
    
    func_table_entry* entry = find_func_entry(table->entries, table->capacity, key);
    b8 is_new_key = entry->key.name.size == 0;
    if (is_new_key && entry->value == nullptr) {
        entry->key = key;
        entry->value = value;
        table->count++;
    } else {
        func_entry_val* c = entry->value;
        b8 found = false;
        while (true) {
            if (value_type_list_equals(&c->param_types, &value->param_types)) {
                found = true;
                break;
            }
            if (c->next == nullptr) break;
            c = c->next;
        }
        if (found) c = value;
        else c->next = value;
    }
    
    return is_new_key;
}

b8 func_hash_table_del(func_hash_table* table, func_entry_key key, string mangled_needle) {
    if (table->count == 0) return false;
    func_table_entry* entry = find_func_entry(table->entries, table->capacity, key);
    if (entry->key.name.size == 0) return false;
    
    func_entry_val* curr = entry->value;
    func_entry_val* prev = curr;
    while (curr != nullptr) {
        if (str_eq(curr->mangled_name, mangled_needle)) {
            if (curr == prev) {
                entry->key = (func_entry_key) {0};
                entry->value = &tombstone;
            }
            
            prev->next = curr->next;
            break;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    return true;
}

b8 func_hash_table_del_full(func_hash_table* table, func_entry_key key) {
    if (table->count == 0) return false;
    func_table_entry* entry = find_func_entry(table->entries, table->capacity, key);
    if (entry->key.name.size == 0) return false;
    entry->key = (func_entry_key) {0};
    entry->value = &tombstone;
    return true;
}

void func_hash_table_add_all(func_hash_table* from, func_hash_table* to) {
    for (u32 i = 0; i < from->capacity; i++) {
        func_table_entry* entry = &from->entries[i];
        if (entry->key.name.size != 0) {
            func_hash_table_set(to, entry->key, entry->value);
        }
    }
}

//~ Struct List
void type_array_init(type_array* array) {
    array->elements = calloc(8, sizeof(P_Container));
    array->count = 0;
    array->capacity = 8;
}

void type_array_free(type_array* array) {
    free(array->elements);
    array->count = 0;
    array->capacity = 0;
}

void type_array_add(type_array* array, P_Container structure) {
    if (array->count + 1 > array->capacity) {
        void* prev = array->elements;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->elements = calloc(array->capacity, sizeof(P_Container));
        memmove(array->elements, prev, array->count * sizeof(P_Container));
        free(prev);
    }
    *(array->elements + array->count) = structure;
    array->count++;
}

//~ Type Modifiers
void type_mod_array_add(M_Arena* arena, type_mod_array* array, P_ValueTypeMod mod) {
    if (array->count + 1 > array->capacity) {
        void* prev = array->elements;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->elements = arena_alloc(arena, array->capacity * sizeof(P_ValueTypeMod));
        memmove(array->elements, prev, array->count * sizeof(P_ValueTypeMod));
    }
    *(array->elements + array->count) = mod;
    array->count++;
}

//~ Expressions
void expr_array_add(M_Arena* arena, expr_array* array, struct P_Expr* mod) {
    if (array->count + 1 > array->capacity) {
        void* prev = array->elements;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->elements = arena_alloc(arena, array->capacity * sizeof(struct P_Expr*));
        memmove(array->elements, prev, array->count * sizeof(struct P_Expr*));
    }
    *(array->elements + array->count) = mod;
    array->count++;
}

//~ Namespaces
void namespace_array_add(M_Arena* arena, namespace_array* array, struct P_Namespace* mod)  {
    if (array->count + 1 > array->capacity) {
        void* prev = array->elements;
        array->capacity = GROW_CAPACITY_BIGGER(array->capacity);
        array->elements = arena_alloc(arena, array->capacity * sizeof(struct P_Namespace*));
        memmove(array->elements, prev, array->count * sizeof(struct P_Namespace*));
    }
    *(array->elements + array->count) = mod;
    array->count++;
}

//~ Usings Stack
void using_stack_push(M_Arena* arena, using_stack* stack, struct P_Namespace* mod) {
    if (stack->tos + 1 > stack->capacity) {
        void* prev = stack->stack;
        stack->capacity = GROW_CAPACITY_BIGGER(stack->capacity);
        stack->stack = arena_alloc(arena, stack->capacity * sizeof(struct P_Namespace*));
        memmove(stack->stack, prev, stack->tos * sizeof(struct P_Namespace*));
    }
    *(stack->stack + stack->tos) = mod;
    stack->tos++;
}

void using_stack_pop(using_stack* stack, struct P_Namespace** ret) {
    stack->tos--;
    if (ret != nullptr)
        *ret = stack->stack[stack->tos];
}

void using_stack_peek(using_stack* stack, struct P_Namespace** ret) {
    *ret = stack->stack[stack->tos - 1];
}
