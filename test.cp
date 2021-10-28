void native printf(string format, ... rest);

int main() {
	long x = 10l;
    lmao(54);
    print((double)54);
    print(10 % 3);
    return 0;
}

int lmao(int x) {
	printf("%d\n", x);
	return x;
}

void print(double count) {
	printf("%f\n", count);
}
