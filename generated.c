#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
typedef const char* string;

int main(void);
long long two_1long_varargs(long long count, ...);

int main(void) {
	long long x = 10;
	printf("test\n", 10);
	two_1long_varargs(1, 0, 10, 29, 39);
	return 0;
}
long long two_1long_varargs(long long count, ...) {
va_list others;
va_start(others, count);
	return count;
va_end(others);
}
