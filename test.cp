import "standard/io.cp";
import "standard/math.cp";

int main() {
    Math.vec2 a;
    a.x = 10;
    a.y = 10;
    Math.vec2 b;
    b.x = 10;
    b.y = 10;
    Math.vec2 c = a + b;
    
    float c0 = c[0];
    
    printf("a = (%.3f, %.3f)\n", a.x, a.y);
    printf("b = (%.3f, %.3f)\n", b.x, b.y);
    printf("c = (%.3f, %.3f)\n", c.x, c.y);
    
    return 0;
}
