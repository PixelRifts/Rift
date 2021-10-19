#include <stdio.h>
#include <stdbool.h>

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
	yolo haha;
	haha.term.t1 = 1;
	foo_2intbool(haha.term.t1, true);
	return 0;
}
