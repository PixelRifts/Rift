#include "vm.h"

DArray_Impl(u8);

VM_Chunk VM_ChunkAlloc(void) {
	VM_Chunk chunk = {0};
	return chunk;
}

void VM_ChunkPush(VM_Chunk* chunk, u8* bytes, u32 count) {
	darray_add_all(u8, chunk, bytes, count);
}

void VM_ChunkFree(VM_Chunk* chunk) {
	darray_free(u8, chunk);
}
