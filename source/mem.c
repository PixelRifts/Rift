#include "mem.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#ifdef CPCOM_WIN
#include <windows.h>
#elif defined(CPCOM_LINUX)
#include <sys/mman.h>
#endif

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))

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

b8 is_power_of_two(uintptr_t x) {
	return (x & (x-1)) == 0;
}

u64 align_forward_u64(u64 ptr, u64 align) {
	u64 p, a, modulo;
    
	assert(is_power_of_two(align));
    
	p = ptr;
	a = (size_t)align;
	// Same as (p % a) but faster as 'a' is a power of two
	modulo = p & (a-1);
    
	if (modulo != 0) {
		// If 'p' address is not aligned, push the address to the
		// next value which is aligned
		p += a - modulo;
	}
	return p;
}


//~ Arena (No alignment)

void* arena_alloc(M_Arena* arena, u64 size) {
    void* memory = 0;
    
    if (arena->alloc_position + size > arena->commit_position) {
        if (!arena->static_size) {
            u64 commit_size = size;
            
            commit_size += M_ARENA_COMMIT_SIZE - 1;
            commit_size -= commit_size % M_ARENA_COMMIT_SIZE;
            
            if (arena->commit_position >= arena->max) {
                assert(0 && "Arena is out of memory");
            } else {
                mem_commit(arena->memory + arena->commit_position, commit_size);
                arena->commit_position += commit_size;
            }
        } else {
            assert(0 && "Static-Size Arena is out of memory");
        }
    }
    
    memory = arena->memory + arena->alloc_position;
    arena->alloc_position += size;
    return memory;
}

void arena_dealloc(M_Arena* arena, u64 size) {
    if (size > arena->alloc_position)
        size = arena->alloc_position;
    arena->alloc_position -= size;
}

void* arena_raise(M_Arena* arena, void* ptr, u64 size) {
    void* raised = arena_alloc(arena, size);
    memcpy(raised, ptr, size);
    return raised;
}

void arena_init(M_Arena* arena) {
    arena->max = M_ARENA_MAX;
    arena->memory = mem_reserve(arena->max);
    arena->alloc_position = 0;
    arena->commit_position = 0;
    arena->static_size = false;
}

void arena_clear(M_Arena* arena) {
    arena_dealloc(arena, arena->alloc_position);
}

void arena_free(M_Arena* arena) {
    mem_release(arena->memory, arena->max);
}

//~ Scratch
typedef struct scratch_free_list_node scratch_free_list_node;
struct scratch_free_list_node {
    scratch_free_list_node* next;
    u32 index;
};

static struct {
    M_Arena arena;
    u32 max_created;
    scratch_free_list_node* free_list;
} scratch_context;

void M_ScratchInit() {
    arena_init(&scratch_context.arena);
}

void M_ScratchFree() {
    arena_free(&scratch_context.arena);
}

M_Scratch scratch_get() {
    if (!scratch_context.free_list) {
        M_Scratch scratch = {0};
        scratch.index = scratch_context.max_created;
        void* ptr = arena_alloc(&scratch_context.arena, M_SCRATCH_SIZE);
        scratch.arena.memory = ptr;
        scratch.arena.max = M_SCRATCH_SIZE;
        scratch.arena.alloc_position = 0;
        scratch.arena.commit_position = M_SCRATCH_SIZE;
        scratch.arena.static_size = true;
        
        scratch_context.max_created++;
        return scratch;
    } else {
        M_Scratch scratch = {0};
        scratch.index = scratch_context.free_list->index;
        
        scratch.arena.memory = (u8*) scratch_context.free_list;
        scratch.arena.max = M_SCRATCH_SIZE;
        scratch.arena.alloc_position = 0;
        scratch.arena.commit_position = M_SCRATCH_SIZE;
        scratch.arena.static_size = true;
        
        scratch_context.free_list = scratch_context.free_list->next;
        return scratch;
    }
}

void scratch_return(M_Scratch* scratch) {
    scratch_free_list_node* prev_head = scratch_context.free_list;
    scratch_context.free_list = (scratch_free_list_node*) scratch->arena.memory;
    scratch_context.free_list->next = prev_head;
    scratch_context.free_list->index = scratch->index;
}

//~ Pool

void* pool_alloc(M_Pool* pool) {
    M_PoolFreeNode* node = pool->free_list;
    
    if (node == nullptr) {
        u64 commit_size = pool->chunk_size * M_POOL_CHUNK_COMMIT_COUNT;
        if (pool->commit_position >= pool->max) {
            assert(0 && "Pool is out of memory");
        } else {
            mem_commit(pool->memory + pool->commit_position, commit_size);
            pool_dealloc_range(pool, pool->memory + pool->commit_position, M_POOL_CHUNK_COMMIT_COUNT);
            pool->commit_position += commit_size;
            node = pool->free_list;
        }
    }
    
    pool->free_list = pool->free_list->next;
    return memset(node, 0, pool->chunk_size);
}

void pool_dealloc(M_Pool* pool, void* ptr) {
    if (ptr == nullptr) return;
    
    M_PoolFreeNode* node;
    
    void* start = pool->memory;
    void* end = pool->memory + pool->commit_position;
    
    if (!(start <= ptr && ptr < end)) {
        assert(0 && "Pointer has not been allocated in this Pool");
        return;
    }
    
    node = (M_PoolFreeNode*) ptr;
    node->next = pool->free_list;
    pool->free_list = node;
}

void pool_dealloc_range(M_Pool* pool, void* ptr, u64 count) {
    void* start = pool->memory;
    void* end = pool->memory + pool->commit_position + pool->chunk_size * count;
    
    for (i64 i = count - 1; i >= 0; i--) {
        void* pointer = ptr + i * pool->chunk_size;
        
        if (!(start <= pointer && pointer < end)) {
            assert(0 && "Overrun while freeing range");
        }
        
        M_PoolFreeNode* node = (M_PoolFreeNode*) pointer;
		node->next = pool->free_list;
		pool->free_list = node;
    }
}

void pool_init(M_Pool* pool, u64 chunk_size) {
    pool->max = M_POOL_MAX;
    pool->memory = mem_reserve(pool->max);
    pool->commit_position = 0;
    
    chunk_size = align_forward_u64(chunk_size, (u64) DEFAULT_ALIGNMENT);
    pool->chunk_size = chunk_size;
    
    assert(chunk_size >= sizeof(M_PoolFreeNode) && "Chunk size is too small");
    pool->free_list = nullptr;
    // pool_clear(pool);
}

void pool_clear(M_Pool* pool) {
    pool_dealloc_range(pool, pool->memory, pool->commit_position / pool->chunk_size);
}

void pool_free(M_Pool* pool) {
    mem_release(pool->memory, pool->max);
}
