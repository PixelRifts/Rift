#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
typedef const char* string;

int main(void);
void test_1intptr(int* i);

int main(void) {
	int x;
	test_1intptr(&x);
	return 10;
}
void test_1intptr(int* i) {
}
