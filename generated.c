#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int main(void);

int lambda0_1int(int x){
printf("%d\n", x);
return x;
	}
int main(void) {
int m = lambda0_1int(10);
printf("Exited successfully");
return 0;
}
