import "standard.cp";
import "examples/add_import.cp";
import "foo.cp";

int main() {
	printf("Started successfully\n");
	int x = 10;
	match (x) {
		case 10: printf("Ten\n");
		case 20: printf("Twenty\n");
		default: printf("wee");
	}
	switch (x) {
		case 10: printf("Ten\n");
		case 20: printf("Twenty\n");
		default: printf("wee");
	}
	printf("Exited successfully\n");
	return 0;
}