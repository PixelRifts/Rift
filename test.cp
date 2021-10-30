void native printf(string format, ... rest);
int* native calloc(int count, int elem_size);
void native free(int* buffer);

int main() {
	int x;
	test(&x);
	return 10;
}

void test(int* i) {
	
}
