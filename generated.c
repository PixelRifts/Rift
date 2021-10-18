#include <stdio.h>
#include <stdbool.h>

typedef struct yolo {
	int t2;
} yolo;
int foo_2intbool(int x, bool y) {
	yolo haha;
	haha.t2 = 2;
	haha.t2 = 8;
	return 0;
}
int main() {
	yolo haha;
	haha.t2 = 1;
	foo_2intbool(haha.t2, true);
	return 0;
}
