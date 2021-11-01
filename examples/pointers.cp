void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

int main() {
	int* x = calloc(16, 4);
	int i = 0;
	while (i < 16) {
		x[i] = i;
		i = i + 1;
	}
	i = 0;
	while (i < 16) {
		printf("%d\n", x[i]);
		i = i + 1;
	}
	free(x);
	return 0;
}
