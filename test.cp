void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

int main() {
	for (int x = 0; x < 10; x = x + 1) {
		printf("Loop%d\n", x);
		continue;
	}
	int x;
	void test() {
		printf("Test\n");
	}
	^void() testLambda = ^() => {
		test();
		printf("Test Lambda\n");
	};
	testLambda();
	printf("Exited successfully");
	return 0;
}
