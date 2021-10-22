#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
typedef const char* string;

int main(void);
int two_1int_varargs(int count, ...);

int main(void) {
	int x = 10;
	printf("%d\n", x);
	return 0;
}
int two_1int_varargs(int count, ...) {
    va_list others;
    va_start(others, count);
    va_end(others);
	return count;
}
