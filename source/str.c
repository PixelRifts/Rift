#include "str.h"
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

string_const str_alloc(M_Arena* arena, u64 size) {
    string_const str = {0};
    str.str = (u8*)arena_alloc(arena, size + 1);
    str.str[size] = '\0';
    str.size = size;
    return str;
}

string_const str_copy(M_Arena* arena, string_const other) {
    string_const str = {0};
    str.str = (u8*)arena_alloc(arena, other.size + 1);
    memcpy(str.str, other.str, other.size);
    str.size = other.size;
    str.str[str.size] = '\0';
    return str;
}

string_const str_cat(M_Arena* arena, string_const a, string_const b) {
    string_const final = {0};
    final.size = a.size + b.size;
    final.str = (u8*)arena_alloc(arena, final.size + 1);
    memcpy(final.str, a.str, a.size);
    memcpy(final.str + a.size, b.str, b.size);
    final.str[final.size] = '\0';
    return final;
}

string_const str_from_format(M_Arena* arena, const char* format, ...) {
    va_list args;
    va_start(args, format);
    const char buf[8092];
    vsnprintf(buf, 8092, format, args);
    va_end(args);
    u64 size = strlen(buf);
    string_const s = str_alloc(arena, size);
    memmove(s.str, buf, size);
    return s;
}

b8 str_eq(string_const a, string_const b) {
    if (a.size != b.size) return false;
    return memcmp(a.str, b.str, b.size) == 0;
}

u64 str_find_first(string_const str, string_const needle, u32 offset) {
    u64 i = 0;
    if (needle.size > 0) {
        i = str.size;
        if (str.size >= needle.size) {
            i = offset;
            i8 c = needle.str[0];
            u64 one_past_last = str.size - needle.size + 1;
            for (; i < one_past_last; i++) {
                if (str.str[i] == c) {
                    if ((str.size - i) >= needle.size) {
                        string_const sub = { .str = str.str + i, .size = needle.size };
                        if (str_eq(sub, needle)) break;
                    }
                }
            }
            if (i == one_past_last) {
                i = str.size;
            }
        }
    }
    return i;
}

void string_list_push_node(string_const_list* list, string_const_list_node* node) {
    if (!list->first && !list->last) {
        list->first = node;
        list->last = node;
    } else {
        list->last->next = node;
        list->last = node;
    }
    list->node_count += 1;
    list->total_size += node->size;
}

void string_list_push(M_Arena* arena, string_const_list* list, string_const str) {
    string_const_list_node* node = (string_const_list_node*) arena_alloc(arena, sizeof(string_const_list_node));
    node->str = str.str;
    node->size = str.size;
    string_list_push_node(list, node);
}

string_const string_list_flatten(M_Arena* arena, string_const_list* list) {
    string_const final = str_alloc(arena, list->total_size);
    u64 current_offset = 0;
    for (string_const_list_node* node = list->first; node != nullptr; node = node->next) {
        memcpy(final.str + current_offset, node->str, node->size);
        current_offset += node->size;
    }
    return final;
}
