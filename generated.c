#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void foo_0(void);
int main(void);

int lambda0_1int(int x){
printf("%d\n", x);
return x;
	}
void foo_0(void){}
int main(void) {
void (*meh)() = foo_0;
int m = lambda0_1int(10);
printf("Exited successfully");
return 0;
}
