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

b8 str_str_node_eq(string_const a, string_const_list_node* b) {
    if (a.size != b->size) return false;
    return memcmp(a.str, b->str, a.size) == 0;
}

b8 str_node_eq(string_const_list_node* a, string_const_list_node* b) {
    if (a->size != b->size) return false;
    return memcmp(a->str, b->str, b->size) == 0;
}

string_const str_replace_all(M_Arena* arena, string_const to_fix, string_const needle, string_const replacement) {
    if (needle.size == 0) return to_fix;
    u64 replaceable = str_substr_count(to_fix, needle);
    if (replaceable == 0) return to_fix;
    
    u64 new_size = (to_fix.size - replaceable * needle.size) + (replaceable * replacement.size);
    string_const ret = str_alloc(arena, new_size);
    
    b8 replaced;
    u64 o = 0;
    for (u64 i = 0; i < to_fix.size;) {
        replaced = false;
        if (to_fix.str[i] == needle.str[0]) {
            if ((to_fix.size - i) >= needle.size) {
                string_const sub = { .str = to_fix.str + i, .size = needle.size };
                if (str_eq(sub, needle)) {
                    // replace this one
                    memmove(ret.str + o, replacement.str, replacement.size);
                    replaced = true;
                }
            }
        }
        
        if (replaced) {
            o += replacement.size;
            i += needle.size;
            continue;
        }
        
        ret.str[o] = to_fix.str[i];
        o++; i++;
    }
    
    return ret;
}

u64 str_substr_count(string_const str, string_const needle) {
    u32 ct = 0;
    u64 idx = 0;
    while (true) {
        idx = str_find_first(str, needle, idx);
        if (idx == str.size)
            break;
        ct++;
        idx++;
    }
    return ct;
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

u64 str_find_last(string_const str, string_const needle, u32 offset) {
    u64 prev = str.size;
    if (offset == 0)
        offset = str.size;
    u64 idx = 0;
    while (true) {
        prev = idx;
        idx = str_find_first(str, needle, idx);
        if (idx >= offset) break;
        idx++;
    }
    return prev;
}

static u32 str_hash(string_const str) {
    u32 hash = 2166136261u;
    for (int i = 0; i < str.size; i++) {
        hash ^= str.str[i];
        hash *= 16777619;
    }
    return hash;
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

b8 string_list_equals(string_const_list* a, string_const_list* b) {
    if (a->total_size != b->total_size) return false;
    if (a->node_count != b->node_count) return false;
    string_const_list_node* curr_a = a->first;
    string_const_list_node* curr_b = b->first;
    
    while (curr_a != nullptr || curr_b != nullptr) {
        if (curr_a->size != curr_b->size) return false;
        if (memcmp(curr_a->str, curr_b->str, curr_b->size) != 0) return false;
        
        curr_a = curr_a->next;
        curr_b = curr_b->next;
    }
    return true;
}

b8 string_list_contains(string_const_list* a, string_const needle) {
    string_const_list_node* curr = a->first;
    while (curr != nullptr) {
        if (str_str_node_eq(needle, curr))
            return true;
        curr = curr->next;
    }
    return false;
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

void string_array_add(string_const_array* array, string data) {
    if (array->len + 1 > array->cap) {
        void* prev = array->elems;
        u32 new_cap = array->cap == 0 ? 8 : array->cap * 2;
        array->elems = calloc(new_cap, sizeof(string));
        memmove(array->elems, prev, array->len * sizeof(string));
        free(prev);
    }
    array->elems[array->len++] = data;
}

string string_array_remove(string_const_array* array, int idx) {
    if (idx >= array->len || idx < 0) return (string) {0};
    string value = array->elems[idx];
    if (idx == array->len - 1) {
        array->len--;
        return value;
    }
    string* from = array->elems + idx + 1;
    string* to = array->elems + idx;
    memmove(to, from, sizeof(string) * (array->len - idx - 1));
    array->len--;
    return value;
}

void string_array_free(string_const_array* array) {
    array->cap = 0;
    array->len = 0;
    free(array->elems);
}
