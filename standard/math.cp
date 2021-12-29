namespace Math {
    struct vec2 { float x; float y; }
    struct vec3 { float x; float y; float z; }
    struct vec4 { float x; float y; float z; float w; }
    
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
    
    float operator[](vec2 a, int index) {
        if (index == 0) return a.x;
        if (index == 1) return a.y;
        return 0;
    }
    
    vec3 operator+(vec3 a, vec3 b) {
        vec3 r;
        r.x = a.x + b.x;
        r.y = a.y + b.y;
        r.z = a.z + b.z;
        return r;
    }
    
    vec3 operator-(vec3 a, vec3 b) {
        vec3 r;
        r.x = a.x - b.x;
        r.y = a.y - b.y;
        r.z = a.z - b.z;
        return r;
    }
    
    vec3 operator*(vec3 a, float s) {
        vec3 r;
        r.x = a.x * s;
        r.y = a.y * s;
        r.z = a.z * s;
        return r;
    }
    
    vec3 operator/(vec3 a, float s) {
        vec3 r;
        r.x = a.x / s;
        r.y = a.y / s;
        r.z = a.z / s;
        return r;
    }
    
    float operator[](vec3 a, int index) {
        if (index == 0) return a.x;
        if (index == 1) return a.y;
        if (index == 2) return a.z;
        return 0;
    }
    
    vec4 operator+(vec4 a, vec4 b) {
        vec4 r;
        r.x = a.x + b.x;
        r.y = a.y + b.y;
        r.z = a.z + b.z;
        r.w = a.w + b.w;
        return r;
    }
    
    vec4 operator-(vec4 a, vec4 b) {
        vec4 r;
        r.x = a.x - b.x;
        r.y = a.y - b.y;
        r.z = a.z - b.z;
        r.w = a.w - b.w;
        return r;
    }
    
    vec4 operator*(vec4 a, float s) {
        vec4 r;
        r.x = a.x * s;
        r.y = a.y * s;
        r.z = a.z * s;
        r.w = a.w * s;
        return r;
    }
    
    vec4 operator/(vec4 a, float s) {
        vec4 r;
        r.x = a.x / s;
        r.y = a.y / s;
        r.z = a.z / s;
        r.w = a.w / s;
        return r;
    }
    
    float operator[](vec4 a, int index) {
        if (index == 0) return a.x;
        if (index == 1) return a.y;
        if (index == 2) return a.z;
        if (index == 3) return a.w;
        return 0;
    }
    
    struct mat2 { float[4]  m; }
    struct mat3 { float[9]  m; }
    struct mat4 { float[16] m; }
}
