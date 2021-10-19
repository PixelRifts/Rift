struct another {
	int t1;
}

struct yolo {
	another term;
}

int foo(int x, bool y) {
	yolo haha;
	haha.term.t1 = 2;
	haha.term.t1 = 8;
	return 0;
}

int main() {
	yolo haha;
	haha.term.t1 = 1;
    foo(haha.term.t1, true);
    return 0;
}
