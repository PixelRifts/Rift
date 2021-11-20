void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

import "blah.cp";

int main() {
	printf("Started successfully\n");
	int m = x(10, 20);
	printf("%d\n", m);
	printf("Exited successfully\n");
	return 0;
}

int foo(int x, int x) {
	
}
