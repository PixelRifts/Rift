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
    
    namespace Inner {
        
        struct jay {
            int k;
            bool l;
            char m;
            long n;
        }
        
    }
    
    void proc() {
        //meh m;
        //assign_ten(m);
        printf("Yeet\n");
    }
}

void assign_ten(TEST.Inner.jay& mu_ref) {
    mu_ref.k = 10;
}

int main() {
    using TEST;
    TEST.Inner.jay yee;
    proc();
    int kek = yesnt.ja;
    assign_ten(yee);
    
    return 0;
}
