void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

void foo() {}

int main() {
	^void() meh = foo;
	int m = ^(int x) => {
		printf("%d\n", x);
		return x;
	}(10);
	printf("Exited successfully");
	return 0;
}
