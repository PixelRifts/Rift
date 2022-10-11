# Rift
Rift - A Language Compiler

Important!
***Master branch is WIP (Compiler rework)***

Check out branch v1 for the old transpiler

Linux support for v2 coming soon :tm:

Meant to be C-Like with some helpful features.
- Syntax is similar to C
- Transpiles to C code

## Getting Started
(NOTE FOR LINUX USERS): Install `llvm llvm-dev zlib1g-dev` before compiling

1) Clone the Repo to your machine
2) Make a bin directory in the repo root
3) Build project using build.bat
4) (temporary) Compile example test.rf to blah.ll, use `clang -g blah.ll -o test.exe` to compile the IR to an executable
