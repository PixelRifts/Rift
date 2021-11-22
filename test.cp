import "standard.cp";
import "examples/add_import.cp";
import "foo.cp";

int main() {
	printf("Started successfully\n");
	int m = add(10, 20);
	for (int i = 0; i < 10; i = i + 1) {
		if (true) { break; }
	}
	printf("%d\n", m);
	printf("Exited successfully\n");
	return 0;
}
