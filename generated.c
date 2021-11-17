#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int main(void);

int main(void) {
for (int x = 0;
(x<10);x = (x+1))
{
printf("Loop%d\n", x);
continue;
		}
printf("Exited successfully");
return 0;
}
