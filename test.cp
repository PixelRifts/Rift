import "standard.cp";

namespace TEST {
    int mu;
    
    struct meh {
        int k;
        bool l;
        char m;
        long n;
    }
}

@!linux
void tagged() {
    printf("NOT Linux");
}

@linux
void tagged() {
    printf("Linux");
}

void assign_ten(int& mu_ref) {
    mu_ref = 10;
}

int main() {
    using TEST;
    meh yes;
    
    assign_ten(mu);
    // tagged();
	
    @!linux {
		printf("This stmtlist was tagged!\n");
	}
	
    return 0;
}
