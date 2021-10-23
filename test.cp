void native printf(string format, ... rest);

int main() {
	long x = 10l;
    printf("test\n", 10);
	two(1l, 0, 10, 29, 39);
    return 0;
}

long two(long count, ... others) {
	return count;
}
