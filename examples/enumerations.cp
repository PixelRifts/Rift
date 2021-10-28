void native printf(string format, ... rest);

enum test_enum {
	first, second, third
}

int main() {
	int f = test_enum.first;
	int s = test_enum.second;
	int t = test_enum.third;
	printf("test_enum: (%d, %d, %d)", f, s, t);
	return 0;
}
