native void printf(string format, ... rest);
native void* calloc(int count, int elem_size);
native void free(void* buffer);

@linux
void tagged() {
	printf("Linux");
}

@windows
void tagged() {
	printf("Windows");
}

int main() {
	tagged();
	return 0;
}
