void native printf(string format, ... rest);
int* native calloc(int count, int elem_size);
void native free(int* buffer);

struct test {
	int x;
	int y;
}

int main() {
	int x;
	int* ptr = &x;
	void* p = ptr;
	test* again = (test*)p;
	test* foo = (test*)nullptr;
	int back_to_int = *(ptr + 5);
	return 10;
}
