void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

int main() {
	^int(int, int) add = ^(int x, int y) => {
		return x + y;
	};
	int c = add(20, 30);
	printf("%d", c);
	return 0;
}
