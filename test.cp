void native printf(string format, ... rest);
void* native calloc(int count, int elem_size);
void native free(void* buffer);

void bazpqr(int x) {
	printf("Called Baz\n");
}

^void(int) barabc(int x) {
	printf("Called Bar. Gave Baz.\n");
	return bazpqr;
}

^^void(int)(int) fooxyz(int x) {
	printf("Called Foo. Gave Bar.\n");
	return barabc;
}

int main() {
	^^^void(int)(int)(int) foo = fooxyz;
	^^^void(int)(int)(int) fooptr = &foo;
	^^void(int)(int) bar = foo(10);
	^void(int) baz = bar(20);
	baz(30);
	return 0;
}
