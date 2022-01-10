#include "mem.h"
#include <string.h>
#include <assert.h>

#ifdef CPCOM_WIN
#include <windows.h>
#elif defined(CPCOM_LINUX)
#include <sys/mman.h>
#endif

static void* mem_reserve(u64 size) {
#ifdef CPCOM_WIN
    void* memory = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
#elif defined(CPCOM_LINUX)
    void* memory = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
    return memory;
}

static void mem_release(void* memory, u64 size) {
#ifdef CPCOM_WIN
    VirtualFree(memory, 0, MEM_RELEASE);
#elif defined(CPCOM_LINUX)
    munmap(memory, size);
#endif
}

static void mem_commit(void* memory, u64 size) {
#ifdef CPCOM_WIN
    VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE);
#elif defined(CPCOM_LINUX)
    mprotect(memory, size, PROT_READ | PROT_WRITE);
#endif
}

static void mem_decommit(void* memory, u64 size) {
#ifdef CPCOM_WIN
    VirtualFree(memory, size, MEM_DECOMMIT);
#elif defined(CPCOM_LINUX)
    mprotect(memory, size, PROT_NONE);
#endif
}

void* arena_alloc(M_Arena* arena, u64 size) {
    void* memory = 0;
    if (arena->alloc_position + size > arena->commit_position) {
        u64 commit_size = size;
        
        commit_size += M_ARENA_COMMIT_SIZE - 1;
        commit_size -= commit_size % M_ARENA_COMMIT_SIZE;
        
        mem_commit(arena->memory + arena->commit_position, commit_size);
        arena->commit_position += commit_size;
    }
    memory = arena->memory + arena->alloc_position;
    arena->alloc_position += size;
    return memory;
}

void arena_dealloc(M_Arena* arena, u64 size) {
    if(size > arena->alloc_position)
        size = arena->alloc_position;
    arena->alloc_position -= size;
}

void arena_init(M_Arena* arena) {
    arena->max = M_ARENA_MAX;
    arena->memory = mem_reserve(arena->max);
    arena->alloc_position = 0;
    arena->commit_position = 0;
}

void arena_clear(M_Arena* arena) {
    arena_dealloc(arena, arena->alloc_position);
}

void arena_free(M_Arena* arena) {
    mem_release(arena->memory, arena->max);
}
