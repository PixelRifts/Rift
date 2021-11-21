# Rift
Rift - A Language Transpiler
- Syntax is similar to C
- Transpiles to C code

## Getting Started
1) Clone the Repo to your machine
2) Make a bin directory in the repo root
3) To regenerate the example, run `./build.bat` and then `bin\cpcom.exe ./test.cp`.

## NOTES
- This project is currently windows only. You can easily add linux support yourself. (The only thing that is windows specific is in `mem.c`). (make sure to remove #error in the linux block in defines.h too)
- You can build the project with either gcc or clang. The default compiler is clang, but you can go into the build script and change the compiler by changing the `cc` variable to `gcc` instead of `clang`

## TODOS
- Nested Comment Blocks
- Exponential notation
- Enums should be Types (Explicitly castable to int)
- Switch Statements
- Operator Overloading
- Inline struct initializers
- Standard Library
- Typedefs
- Preprocessor stuff 
