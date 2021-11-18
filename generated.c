#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int foo_0(void);
int (*getFoo_1int(int wee))();
int main(void);

int foo_0(void) {
printf("Foo called\n");
return 69;
}
int (*getFoo_1int(int wee))() {
printf("%d\n", wee);
return foo_0;
}
int main(void) {
int m = getFoo_1int(10)();
printf("Exited successfully");
return 0;
}
