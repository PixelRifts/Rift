void native printf(string format, ... rest);

int main() {
	long x = 10l;
    printf("test\n", 10);
    lmao(54);
	two(1, 0, 10, 29, 39);
    return 0;
}

int lmao(int x) {
	printf("%d\n", x);
	return x;
}

long two(long count, ... others) {
	return count;
}
