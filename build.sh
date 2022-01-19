#!/bin/sh

cc=gcc

rm ./bin/generated generated.c

c_filenames=./source/*.c

compiler_flags=-Wall\ -Wvarargs\ -Werror\ -Wno-unused-function\ -Wno-format-security\ -Wno-discarded-qualifiers\ -Wno-pointer-to-int-cast
include_flags=-Isource\ -Ithird-party/include
# shell32 is for windows' cmd, and therefore not included here
# @Hilbert_Curve you may need linux binaries for llvm-c here
linker_flags=-g -Lthird-party/lib
defines=-D_DEBUG\ -D_CRT_SECURE_NO_WARNINGS
output=-o\ bin/rift

mkdir -p bin

echo "Building rift executable..."

${cc} ${c_filenames} ${compiler_flags} ${defines} ${include_flags} ${linker_flags} ${output}
