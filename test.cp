void native printf(string format, ... data);

int main() {
	int x = 10;
    printf("%d\n", x);
    return 0;
}

int two(int count, ... others) {
	return count;
}
