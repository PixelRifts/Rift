void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

int foo() {
	printf("Foo called\n");
	return 69;
}

^int() getFoo(int wee) {
	printf("%d\n", wee);
	return foo;
}

int main() {
	int m = getFoo(10)();
	printf("Exited successfully");
	return 0;
}
