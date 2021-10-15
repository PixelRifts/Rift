REM build script for engine
@ECHO off
SetLocal EnableDelayedExpansion

REM Get's list of all C files
SET c_filenames= 
FOR /R %%f in (*.c) do (
	SET c_filenames=!c_filenames! %%f
)

SET compiler_flags=-Wall -Wvarargs -Werror -Wno-unused-function
SET include_flags=-Isource -Ilib/include
SET linker_flags=-g -lshell32
SET defines=-D_DEBUG -DCPLEXER -D_CRT_SECURE_NO_WARNINGS
SET output=-o bin/cpcom.exe

ECHO "Building cpcom.exe..."
clang %c_filenames% %compiler_flags% %defines% %include_flags% %linker_flags% %output%