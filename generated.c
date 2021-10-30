#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
typedef const char* string;

int main(void);

typedef struct test {
	int x;
	int y;
} test;
int main(void) {
	int x;
	int* ptr = &x;
	void* p = ptr;
	test* again = (test*)p;
	test* foo = (test*)((void*)0);
	int back_to_int = *(ptr+5);
	return 10;
}
