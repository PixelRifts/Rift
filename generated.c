#include <stdio.h>
#include <stdbool.h>
typedef const char* string;

typedef struct another {
	int t1;
} another;
typedef struct yolo {
	another term;
} yolo;
int foo_2intbool(int x, bool y) {
	yolo haha;
	haha.term.t1 = 2;
	haha.term.t1 = 8;
	return 0;
}
int main() {
	string haha;
	haha = "Testing";
	int x;
	x = test();
	foo_2intbool(x, true);
	return 0;
}
