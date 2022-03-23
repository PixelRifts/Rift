/* date = September 27th 2021 11:45 am */

#ifndef MEM_H
#define MEM_H

#include "defines.h"
#include <stdlib.h>

#define Gigabytes(count) (u64) (count * 1024 * 1024 * 1024)
#define Megabytes(count) (u64) (count * 1024 * 1024)
#define Kilobytes(count) (u64) (count * 1024)

//~ Arena (Linear Allocator)

typedef struct M_Arena {
    u8* memory;
    u64 max;
    u64 alloc_position;
    u64 commit_position;
    b8 static_size;
} M_Arena;

#define M_ARENA_MAX Gigabytes(1)
#define M_ARENA_COMMIT_SIZE Kilobytes(8)

void* arena_alloc(M_Arena* arena, u64 size);
void arena_dealloc(M_Arena* arena, u64 size);
void* arena_raise(M_Arena* arena, void* ptr, u64 size);

void arena_init(M_Arena* arena);
void arena_clear(M_Arena* arena);
void arena_free(M_Arena* arena);

//~ Scratch Helpers
// A scratch block is just a view into an arena

typedef struct M_Scratch {
    // This is just a view into the bigger main arena. THIS THING SHOULD NOT GROW
    M_Arena arena;
    u32 index;
} M_Scratch;

#define M_SCRATCH_SIZE Kilobytes(4)

// Should be called at the start of your app
void M_ScratchInit();
// Should be called at the end of your app
void M_ScratchFree();

M_Scratch scratch_get();
void scratch_return(M_Scratch* scratch);

//~ Pool Allocator

typedef struct M_PoolFreeNode {
    struct M_PoolFreeNode* next;
} M_PoolFreeNode;

typedef struct M_Pool {
    u8* memory;
    u64 max;
    u64 commit_position;
    u64 chunk_size;
    
    M_PoolFreeNode* free_list;
} M_Pool;

#define M_POOL_MAX Gigabytes(1)
#define M_POOL_CHUNK_COMMIT_COUNT 4

void* pool_alloc(M_Pool* pool);
void pool_dealloc(M_Pool* pool, void* ptr);
void pool_dealloc_range(M_Pool* pool, void* ptr, u64 count);

void pool_init(M_Pool* pool, u64 chunk_size);
void pool_clear(M_Pool* pool);
void pool_free(M_Pool* pool);

// TODO(voxel): Bump / Stack Allocator

#endif //MEM_H
