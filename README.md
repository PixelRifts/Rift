# Rift
Rift - A Language Compiler

Important!
***Master branch is WIP (Compiler rework)***


Check out branch v1 for the old transpiler

Meant to be C-Like with some helpful features.
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

## Features
- A lot of stuff carried over from C
- Nested Comment blocks
- A better syntax for function pointers
- Lambdas
- Function overloading
- Match-Case statements (so people like me who forget writing breaks after switch cases, don't have to)
- Operator overloading
- Namespaces (Like C++)
- Simple References (Like C++)
- A Tags system that replaces #define and #ifdef/#ifndef
- Flag Enums (for nicer bitfield declarations)
- Imports and "Modules"
- Transpiles to a single compilation unit.

## NOTES
- This project is currently windows and linux only.
- You can build the project with llvm clang on windows or gcc on linux.

## TODOS
- Exponential notation
- Enums should be Types (Explicitly castable to int)
- Standard Library (WIP)
- LLVM Backend
- Maybe x64 Backend
