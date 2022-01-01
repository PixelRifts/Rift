import "standard/io.cp";

namespace Yesno {
    
    enum AnEnum {
        a = 7,
        b,
        c = b * 2,
        d
    }
    
    flagenum AFlagEnum {
        bit1,
        bit2,
        bit3,
        bit1and2 = bit1 | bit2,
        bit4,
        bit5,
    }
    
}

int main() {
    using Yesno;
    
    printf("a: %d\n", AnEnum.a);
    printf("b: %d\n", AnEnum.b);
    printf("c: %d\n", AnEnum.c);
    printf("d: %d\n", AnEnum.d);
    printf("%d\n", AnEnum.count);
    
    return 0;
}
