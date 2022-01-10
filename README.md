# Rift
Rift - A Language Transpiler
- Syntax is similar to C
- Transpiles to C code

## Getting Started
1) Clone the Repo to your machine
2) Make a bin directory in the repo root
3) To regenerate the example:
    a) On Windows, run `./build.bat` and then `bin\rift.exe ./test.rf`.
    b) On Linux, run `./build.sh` and then `./bin/rift ./test.rf`.
4) To run the example:
    a) On Linux, run `./run.sh`.

## NOTES
- This project is currently windows and linux only.
- You can build the project with llvm clang on windows or gcc on linux.

## TODOS
- Nested Comment Blocks
- Exponential notation
- Enums should be Types (Explicitly castable to int)
- Inline struct initializers
- Standard Library
- Preprocessor stuff 
