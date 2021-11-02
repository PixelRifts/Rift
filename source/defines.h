/* date = September 27th 2021 11:48 am */

#ifndef DEFINES_H
#define DEFINES_H

// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef char b8;

#define true 1
#define false 0

#define null 0
#define nullptr (void*)0
#define trace printf("Trace\n")

#define REDUCTION_ERROR -1

#define FATAL(s)            \
do {                    \
fprintf(stderr, s); \
exit(-10);          \
} while(false)

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define CPCOM_WIN
#elif defined(__linux__) || defined(__gnu_linux__)
#define CPCOM_LINUX
#error "The compiler only supports windows for now"
#else
#error "The compiler only supports windows for now"
#endif

#endif //DEFINES_H
