/* date = September 27th 2021 11:45 am */

#ifndef MEM_H
#define MEM_H

#include "defines.h"
#include <stdlib.h>

//~ Arena (Linear Allocator)

typedef struct M_Arena {
    u8* memory;
    u64 max;
    u64 alloc_position;
    u64 commit_position;
} M_Arena;

#define Gigabytes(count) (u64) (count * 1024 * 1024 * 1024)
#define Megabytes(count) (u64) (count * 1024 * 1024)
#define Kilobytes(count) (u64) (count * 1024)

#define M_ARENA_MAX Gigabytes(1)
#define M_ARENA_COMMIT_SIZE Kilobytes(4)

void* arena_alloc(M_Arena* arena, u64 size);
void arena_dealloc(M_Arena* arena, u64 size);

void arena_init(M_Arena* arena);
void arena_clear(M_Arena* arena);
void arena_free(M_Arena* arena);

#endif //MEM_H
