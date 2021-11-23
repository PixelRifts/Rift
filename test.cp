import "standard.cp";
import "examples/add_import.cp";
import "foo.cp";

@native struct meh;

int main() {
	printf("Started successfully\n");

	meh we;
	we.no_memberchecking_here = "haha yes";

	printf("Exited successfully\n");
	return 0;
}
