#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int add_2intint(int a, int b);
void bar_0(void);
void foo_0(void);
int main(void);

int add_2intint(int a, int b) {
return (a+b);
}
void bar_0(void) {
printf("Bar\n");
}
void foo_0(void) {
printf("Foo\n");
}
int main(void) {
printf("Started successfully\n");
int m = add_2intint(10, 20);
printf("%d\n", m);
printf("Exited successfully\n");
return 0;
}
