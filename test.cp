struct yolo {
	int t2;
}

int foo(int x, bool y) {
	yolo haha;
	haha.t2 = 2;
	haha.t2 = 8;
	return 0;
}

int main() {
	yolo haha;
	haha.t2 = 1;
    foo(haha.t2, true);
    return 0;
}
