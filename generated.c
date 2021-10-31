#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>

int main(void);
int** test_1intptrptr(int** blah);

int main(void) {
	int (*x[4]);
	int (**xp) = x;
	test_1intptrptr(x);
	return 0;
}
int** test_1intptrptr(int (**blah)) {
	return blah;
}
