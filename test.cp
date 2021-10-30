void native printf(string format, ... rest);
int* native calloc(int count, int elem_size);
void native free(int* buffer);

int main() {
	int* buffer = calloc(16, 4);
	buffer[2] = 10.f;
	int i = 0;
	while (i < 16) {
		printf("buffer[%d] = %d\n", i, buffer[i]);
		i = i + 1;
	}
	free(buffer);
	
	return 0;
}
