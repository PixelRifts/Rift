#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int main(void);

void maintest_0(void) {
printf("Test\n");
}
void lambda0_0(void){
maintest_0();
printf("Test Lambda\n");
	}
int main(void) {
for (int x = 0;
(x<10);x = (x+1))
{
printf("Loop%d\n", x);
continue;
		}
int x;
void (*testLambda)() = lambda0_0;
testLambda();
printf("Exited successfully");
return 0;
}
