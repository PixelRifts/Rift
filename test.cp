import "standard/io.cp";

namespace Math {
    struct vec2 {
        float x;
        float y;
    }
}

Math.vec2 operator+(Math.vec2 a, Math.vec2 b) {
    Math.vec2 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return r;
}

float operator[](Math.vec2 a, int index) {
    if (index == 0) return a.x;
    if (index == 1) return a.y;
    return 0;
}

int main() {
    Math.vec2 a;
    a.x = 10;
    a.y = 10;
    Math.vec2 b;
    b.x = 10;
    b.y = 10;
    Math.vec2 c = a + b;
    
    printf("a = (%f, %f)\n", a.x, a.y);
    printf("b = (%f, %f)\n", b.x, b.y);
    printf("c = (%f, %f)\n", c.x, c.y);
    printf("c0 = %f\n", c0);
    
    return 0;
}
