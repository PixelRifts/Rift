#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
typedef const char* string;

int main(void);

int main(void) {
	int i = 1;
	while ((i<=100))
		{
			if (((i%3)==0))
				printf("Fizz");
			if (((i%5)==0))
				printf("Buzz");
			if ((((i%3)!=0)&&((i%5)!=0)))
				printf("%d", i);
			printf("\n");
			i = (i+1);
}
	return 0;
}
