void native printf(string format, ... rest);

struct test_structure {
	int some_member;
	float other_member;
}

int main() {
	test_structure s;
	s.some_member = 10;
	s.other_member = 54.f;
	printf("test_structure: (%d, %f)", s.some_member, s.other_member);
	return 0;
}
