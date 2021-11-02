void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

int main() {
	int x = 0;
	change(x);
	printf("%d\n", x);
	return 0;
}

void change(int& r) {
	r = 10;
}
