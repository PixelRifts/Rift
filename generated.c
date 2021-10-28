#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
typedef const char* string;

int main(void);

typedef int test_enum;
enum test_enum {
	_enum_test_enum_first,
	_enum_test_enum_second,
	_enum_test_enum_third,
};
int main(void) {
	int f = _enum_test_enum_first;
	int s = _enum_test_enum_second;
	int t = _enum_test_enum_third;
	printf("test_enum: (%d, %d, %d)", f, s, t);
	return 0;
}
