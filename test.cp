import "standard.cp";

namespace TEST {
    int mu;
    
    struct meh {
        int k;
        bool l;
        char m;
        long n;
    }
    
    enum yesnt {
        yes,
        no,
        ja,
        nein
    }
    
    void proc() {
        
    }
}

struct binomial {
    int k;
}

void assign_ten(TEST.meh& mu_ref) {
    mu_ref.k = 10;
}

int main() {
    TEST.meh yes;
    TEST.proc();
    int kek = TEST.yesnt.ja;
    //assign_ten(yes);
    
    return 0;
}
