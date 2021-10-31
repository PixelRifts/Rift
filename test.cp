void native printf(string format, ... rest);
int* native calloc(int count, int elem_size);
void native free(int* buffer);

int main() {
	int*[4] x;
	int** xp = x;
	test(x);
	return 0;
}

int** test(int** blah) {
	return blah;
}
