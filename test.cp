import "standard.cp";
import "examples/add_import.cp";
import "foo.cp";

int main() {
	printf("Started successfully\n");
	int m = add(10, 20);
	printf("%d\n", m);
	printf("Exited successfully\n");
	return 0;
}
