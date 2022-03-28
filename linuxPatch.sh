#!/bin/sh

sudo apt install llvm llvm-dev sed
sed '43s/.*/static b8 DEBUG_MODE = false;/' source/llvm_backend.c > source/llvm_backend__tmp__.c
rm source/llvm_backend.c
mv source/llvm_backend__tmp__.c source/llvm_backend.c 
