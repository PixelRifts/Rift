native void* malloc(long size);
native void* calloc(int count, int elem_size);
native void* memset(void* block, int ch, int size);
native void  memcpy(void* to, void* from, int size);
native void  memmove(void* to, void* from, int size);
native void  free(void* buffer);
