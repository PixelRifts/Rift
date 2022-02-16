/* date = February 16th 2022 0:27 pm */

#ifndef DS_H
#define DS_H

#include "defines.h"
#include "str.h"
#include "mem.h"

#define Array_GrowCapacity(x) ((x) <= 0 ? 8 : x * 2)

#define Array_Prototype(Name, Data)          \
typedef struct Name {                    \
u32 cap;                             \
u32 len;                             \
Data* elems;                         \
} Name;                                  \
void Name##_add(Name* array, Data data); \
Data Name##_remove(Name* array, int idx);


#define Array_Impl(Name, Data)                                     \
void Name##_add(Name* array, Data data) {                      \
if (array->len + 1 > array->cap) {                         \
void* prev = array->elems;                             \
u32 new_cap = Array_GrowCapacity(array->cap);          \
array->elems = calloc(new_cap, sizeof(Data));          \
memmove(array->elems, prev, array->len * sizeof(Data));\
free(prev);                                            \
}                                                          \
array->elems[array->len++] = data;                         \
}                                                              \
Data Name##_remove(Name* array, int idx) {                     \
if (idx >= array->len || idx < 0) return (Data){0};        \
Data value = array->elems[idx];                            \
if (idx == array->len - 1) {                               \
array->len--;                                          \
return value;                                          \
}                                                          \
Data* from = array->elems + idx + 1;                       \
Data* to = array->elems + idx;                             \
memmove(to, from, sizeof(Data) * (array->len - idx - 1));  \
array->len--;\
return value;\
}

#define Array_Iterate(array, var) for (int var = 0; var < array.len; var++)
#define ArrayPtr_Iterate(array, var) for (int var = 0; var < array->len; var++)

#endif //DS_H
