#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int main(void);

int lambda0_2intint(int x, int y)	{
		return (x+y);
}
int main(void) {
	int (*add)(int , int ) = lambda0_2intint;
	int c = add(10, 20);
	printf("%d", c);
	return 0;
}
