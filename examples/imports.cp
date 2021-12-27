native void printf(string format, ... rest);
native void* calloc(int count, int elem_size);
native void free(void* buffer);

import "./add_import.cp";

int main() {
	printf("Started successfully\n");
	int m = add(10, 20);
	printf("%d\n", m);
	printf("Exited successfully\n");
	return 0;
}
