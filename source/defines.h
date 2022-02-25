/* date = September 27th 2021 11:48 am */

#ifndef DEFINES_H
#define DEFINES_H

// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

// Regular int types.
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef char b8;

#define true 1
#define false 0

#define null 0

#ifndef __cplusplus
#define nullptr (void*)0
#endif

#define trace printf("Trace\n")
#define unreachable printf("How did we get here? In %s on line %d\n", __FILE_NAME__, __LINE__)

#define FATAL(s)            \
do {                    \
fprintf(stderr, s); \
exit(-10);          \
} while(false)

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#  define CPCOM_WIN
#elif defined(__linux__) || defined(__gnu_linux__)
#  define CPCOM_LINUX
#else
#  error "The compiler only supports windows and linux for now"
#endif
#define PATH_MAX 4096

#ifdef CPCOM_WIN
#  include <direct.h>
#  define get_cwd _getcwd
#elif defined(CPCOM_LINUX)
#  include <unistd.h>
#  define get_cwd getcwd
#endif

#endif //DEFINES_H