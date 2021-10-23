#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
typedef const char* string;

int main(void);
int two_1int_varargs(int count, ...);

int main(void) {
	int x = 10;
	printf("test\n", 10);
	two_1int_varargs(1, 0, 10, 29, 39);
	return 0;
}
int two_1int_varargs(int count, ...) {
va_list others;
va_start(others, count);
	return count;
va_end(others);
}
