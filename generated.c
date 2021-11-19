#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int main(void);
int x_2intint(int y, int z);

int main(void) {
printf("Started successfully\n");
int m = x_2intint(10, 20);
printf("%d\n", m);
printf("Exited successfully\n");
return 0;
}
int x_2intint(int y, int z) {
return (y+z);
}
