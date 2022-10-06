/* date = October 6th 2022 7:05 pm */

#ifndef VM_H
#define VM_H

#include "ast_nodes.h"
#include "base/ds.h"

DArray_Prototype(u8);

typedef darray(u8) VM_Chunk;

#if 0
typedef struct VM_Chunk VM_Chunk;
#endif

VM_Chunk VM_ChunkAlloc(void);
void VM_ChunkPush(VM_Chunk* chunk, u8* bytes, u32 count);
void VM_ChunkFree(VM_Chunk* chunk);

#endif //VM_H
