/* date = February 16th 2022 0:27 pm */

#ifndef DS_H
#define DS_H

#include "defines.h"
#include "str.h"
#include "mem.h"

#define DoubleCapacity(x) ((x) <= 0 ? 8 : x * 2)

#define Iterate(array, var) for (int var = 0; var < array.len; var++)
#define IteratePtr(array, var) for (int var = 0; var < array->len; var++)

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
u32 new_cap = DoubleCapacity(array->cap);          \
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

#define Stack_Prototype(Name, Data)          \
typedef struct Name {                    \
u32 cap;                             \
u32 len;                             \
Data* elems;                         \
} Name;                                  \
void Name##_push(Name* stack, Data data);\
Data Name##_pop(Name* stack);            \
Data Name##_peek(Name* stack);


#define Stack_Impl(Name, Data)                                     \
void Name##_push(Name* stack, Data data) {                     \
if (stack->len + 1 > stack->cap) {                         \
void* prev = stack->elems;                             \
u32 new_cap = DoubleCapacity(stack->cap);              \
stack->elems = calloc(new_cap, sizeof(Data));          \
memmove(stack->elems, prev, stack->len * sizeof(Data));\
free(prev);                                            \
}                                                          \
stack->elems[stack->len++] = data;                         \
}                                                              \
Data Name##_pop(Name* stack) {                                 \
if (stack->len == 0) return (Data){0};                     \
return stack->elems[--stack->len];                         \
}                                                              \
Data Name##_peek(Name* stack) {                                \
if (stack->len == 0) return (Data){0};                     \
return stack->elems[stack->len - 1];                       \
}


#endif //DS_H
