native void printf(string format, ... rest);

struct vec2 {
    float x;
    float y;
}

vec2 operator+(vec2 a, vec2 b) {
    vec2 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return r;
}

vec2 operator-(vec2 a, vec2 b) {
    vec2 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    return r;
}

vec2 operator*(vec2 a, float s) {
    vec2 r;
    r.x = a.x * s;
    r.y = a.y * s;
    return r;
}

vec2 operator/(vec2 a, float s) {
    vec2 r;
    r.x = a.x / s;
    r.y = a.y / s;
    return r;
}

int main() {
    vec2 a;
    a.x = 20;
    a.y = 20;
    
    vec2 b;
    b.x = 30;
    b.y = 30;
    
    vec2 sum  = a + b;
    vec2 diff = a - b;
    vec2 atimes2 = a * 2;
    vec2 adiv2 = a / 2;
    
    printf("a = (%f, %f)\n", a.x, a.y);
    printf("b = (%f, %f)\n", b.x, b.y);
    printf("a + b = (%f, %f)\n", sum.x, sum.y);
    printf("a - b = (%f, %f)\n", diff.x, diff.y);
    printf("a * 2 = (%f, %f)\n", atimes2.x, atimes2.y);
    printf("a / 2 = (%f, %f)\n", adiv2.x, adiv2.y);
    return 0;
}
