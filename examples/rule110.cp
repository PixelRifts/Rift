void native printf(string format, ... rest);
void* native malloc(int size);
void* native memset(void* block, int ch, int size);
void* native calloc(int count, int elem_size);
void native memcpy(void* to, void* from, int size);
void native free(void* buffer);

int main() {
	// Size of buffer is 50
	int* prev = malloc(4 * 50);
	int* buffer = malloc(4 * 50);
	prev[49] = 1;
	print_buffer(prev);
	int i = 0;
	while (i < 50) {
		int k = 0;
		while (k < 50) {
			int left;
			int right;
			int mid = prev[k];

			if (k != 0) left = prev[k - 1];
			else left = 0;
			if (k != 49) right = prev[k + 1];
			else right = 0;
			if (left == 1) {
				if (right == 1) {
					if (mid == 1) {
						buffer[k] = 0; // 1 1 1 => 0
					} else {
						buffer[k] = 1; // 1 0 1 => 1
					}
				} else {
					if (mid == 1) {
						buffer[k] = 1; // 1 1 0 => 1
					} else {
						buffer[k] = 0; // 1 0 0 => 0
					}
				}
			} else {
				if (right == 1) {
					if (mid == 1) {
						buffer[k] = 1; // 0 1 1 => 1
					} else {
						buffer[k] = 1; // 0 0 1 => 1
					}
				} else {
					if (mid == 1) {
						buffer[k] = 1; // 0 1 0 => 1
					} else {
						buffer[k] = 0; // 0 0 0 => 0
					}
				}
			}
			k = k + 1;
		}
		memcpy(prev, buffer, 4 * 50);
		print_buffer(buffer);
		i = i + 1;
	}

	free(prev);
	free(buffer);
	return 0;
}

void print_buffer(int* buffer) {
	int i = 0;
	while (i < 50) {
		if (buffer[i] == 1)
			printf("*");
		else
			printf(" ");
		i = i + 1;
	}
	printf("\n");
}
