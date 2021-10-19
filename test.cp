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

int native test();

int main() {
	string haha;
	haha = "Testing";
	int x;
	x = test();
    foo(x, true);
    return 0;
}
