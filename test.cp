bool foo(int x, bool y) {
	if ((1 + 2) == (2 + 1)) {
		return false;
	}
	return true;
}

int main() {
	foo(1, true);
	return 0;
}
