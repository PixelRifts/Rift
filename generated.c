#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
typedef const char* string;

int main(void);
void foo_1double(double d);
void foo_1long(long long d);

int main(void) {
	foo_1double(10.000000);
	foo_1long(10);
	return 0;
}
void foo_1double(double d) {
	printf("Double: %f\n", d);
}
void foo_1long(long long d) {
	printf("Long: %lld\n", d);
}
