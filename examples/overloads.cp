void native printf(string format, ... rest);

int main() {
	foo(10.d);
	foo(10l);
	return 0;
}

void foo(double d) {
	printf("Double: %f\n", d);
}

void foo(long d) {
	printf("Long: %lld\n", d);
}