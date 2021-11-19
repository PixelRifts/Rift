#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int main(void);

int lambda0_2intint(int x, int y){
return (x+y);
	}
int lambda1_2intint(int x, int y){
return (x*y);
	}
int main(void) {
int (*add)(int , int ) = lambda0_2intint;
int multiplied = lambda1_2intint(16, 16);
int c = add(20, 30);
printf("%d, %d", c, multiplied);
return 0;
}
