#include "types.h"

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
