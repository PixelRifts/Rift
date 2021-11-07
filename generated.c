#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void bazpqr_1int(int x);
void (*barabc_1int(int x))(int );
void (*(*fooxyz_1int(int x))(int ))(int );
int main(void);

void bazpqr_1int(int x) {
	printf("Called Baz\n");
}
void (*barabc_1int(int x))(int ) {
	printf("Called Bar. Gave Baz.\n");
	return bazpqr_1int;
}
void (*(*fooxyz_1int(int x))(int ))(int ) {
	printf("Called Foo. Gave Bar.\n");
	return barabc_1int;
}
int main(void) {
	void (*(*(*foo)(int ))(int ))(int ) = fooxyz_1int;
	void (*(*(*fooptr)(int ))(int ))(int ) = &foo;
	void (*(*bar)(int ))(int ) = foo(10);
	void (*baz)(int ) = bar(20);
	baz(30);
	return 0;
}
