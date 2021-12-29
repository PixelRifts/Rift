#include "types.h"

const P_ValueType ValueType_Invalid = { .base_type = str_lit("INVALID"), .full_type = str_lit("INVALID"), .mods = nullptr };

const P_ValueType ValueType_Any = value_type_abs_nc("any");
const P_ValueType ValueType_Integer = value_type_abs_nc("int");
const P_ValueType ValueType_Long = value_type_abs_nc("long");
const P_ValueType ValueType_Float = value_type_abs_nc("float");
const P_ValueType ValueType_Double = value_type_abs_nc("double");

const P_ValueType ValueType_String = value_type_abs_nc("string");
const P_ValueType ValueType_Char = value_type_abs_nc("char");
const P_ValueType ValueType_Bool = value_type_abs_nc("bool");
const P_ValueType ValueType_Void = value_type_abs_nc("void");
const P_ValueType ValueType_Tombstone = value_type_abs_nc("tombstone");

// Mods get filled in in the init routine
P_ValueType ValueType_VoidPointer = {
    .base_type = str_lit("void"),
    .full_type = str_lit("void*"),
    .mod_ct = 1,
};

void types_init(M_Arena* arena) {
    P_ValueTypeMod* void_ptr_mods = arena_alloc(arena, sizeof(P_ValueTypeMod));
    void_ptr_mods->type = ValueTypeModType_Pointer;
    ValueType_VoidPointer.mods = void_ptr_mods;
    ValueType_VoidPointer.mod_ct = 1;
}

void value_type_list_push_node(value_type_list* list, value_type_list_node* node) {
    if (!list->first && !list->last) {
        list->first = node;
        list->last = node;
    } else {
        list->last->next = node;
        list->last = node;
    }
    list->node_count += 1;
}

void value_type_list_push(M_Arena* arena, value_type_list* list, P_ValueType type) {
    value_type_list_node* node = (value_type_list_node*) arena_alloc(arena, sizeof(value_type_list_node));
    node->type = type;
    value_type_list_push_node(list, node);
}

b8 value_type_list_equals(value_type_list* a, value_type_list* b) {
    if (a->node_count != b->node_count) return false;
    value_type_list_node* curr_a = a->first;
    value_type_list_node* curr_b = b->first;
    
    while (curr_a != nullptr || curr_b != nullptr) {
        if (curr_a->type.full_type.size != curr_b->type.full_type.size) return false;
        if (memcmp(curr_a->type.full_type.str, curr_b->type.full_type.str, curr_b->type.full_type.size) != 0) return false;
        
        curr_a = curr_a->next;
        curr_b = curr_b->next;
    }
    return true;
}

b8 is_ptr(P_ValueType* a) {
    if (a->mod_ct == 0) return false;
    if (a->mods[a->mod_ct - 1].type != ValueTypeModType_Pointer) return false;
    return true;
}

b8 is_array(P_ValueType* a) {
    if (a->mod_ct == 0) return false;
    if (a->mods[a->mod_ct - 1].type != ValueTypeModType_Array) return false;
    return true;
}

b8 is_ref(P_ValueType* a) {
    if (a->mod_ct == 0) return false;
    if (a->mods[a->mod_ct - 1].type != ValueTypeModType_Reference) return false;
    return true;
}
