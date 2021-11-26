import "standard.cp";

namespace booyeah {
	void booyah() {}
}

@!linux
void tagged() {
	printf("NOT Linux");
}

@linux
void tagged() {
	printf("Linux");
}

int main() {
	printf("Started successfully\n");
	tagged();

	@!linux {
		printf("This stmtlist was tagged!\n");
	}

	printf("Exited successfully\n");
	return 0;
}
