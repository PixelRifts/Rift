#include <stdio.h>
#include <stdbool.h>

bool foo_2intbool(int x, bool y) {
	if (((1+2)==(2+1)))
		{
			return false;
}
	return true;
}
int main() {
	foo_2intbool(1, true);
	return 0;
}
