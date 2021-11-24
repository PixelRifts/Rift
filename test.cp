import "standard.cp";
// import "examples/add_import.cp";
// import "foo.cp";

@linux
void tagged() {
	printf("Linux");
}

@windows
void tagged() {
	printf("Windows");
}

int main() {
	printf("Started successfully\n");
	tagged();
	printf("Exited successfully\n");
	return 0;
}
