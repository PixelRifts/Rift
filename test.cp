void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

int main() {
	printf("Started successfully");
	printf("Exited successfully");
	return 0;
}
