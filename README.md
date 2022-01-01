# Rift
Rift - A Language Transpiler
- Syntax is similar to C
- Transpiles to C code

## Getting Started
1) Clone the Repo to your machine
2) Make a bin directory in the repo root
3) To regenerate the example, run `./build.bat` and then `bin\rift.exe ./test.rf`.

## NOTES
- This project is currently windows only. You can easily add linux support yourself. (The only thing that is windows specific is in `mem.c`). (make sure to remove #error in the linux block in defines.h too)
- You can build the project with llvm clang on windows or gcc on linux

## TODOS
- Nested Comment Blocks
- Exponential notation
- Enums should be Types (Explicitly castable to int)
- Inline struct initializers
- Standard Library
- Preprocessor stuff 
