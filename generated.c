#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
typedef const char* string;

int main(void);
int lmao_1int(int x);
long long two_1long_varargs(long long count, ...);

int main(void) {
	long long x = 10;
	printf("test\n", 10);
	lmao_1int(54);
	two_1long_varargs(1, 0, 10, 29, 39);
	return 0;
}
int lmao_1int(int x) {
	printf("%d\n", x);
	return x;
}
long long two_1long_varargs(long long count, ...) {
va_list others;
va_start(others, count);
	return count;
va_end(others);
}
