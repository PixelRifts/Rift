native void printf(string format, ... rest);
native void* calloc(int count, int elem_size);
native void free(void* buffer);

int sub(int a, int b) {
	return a - b;
}

int main() {
	^int(int, int) add = ^(int x, int y) => {
		return x + y;
	};
	^int(int, int) sub = sub;
	int multiplied = ^(int x, int y) => {
		return x * y;
	}(16, 16);
	int s = sub(10, 20);
	int c = add(20, 30);
	printf("%d, %d, %d", c, s, multiplied);
	return 0;
}
