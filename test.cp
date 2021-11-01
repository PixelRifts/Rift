void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

int main() {
	int* x = calloc(4 * 4);
	int fourth = x[3];
	return 0;
}
