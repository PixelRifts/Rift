@ECHO off
SetLocal EnableDelayedExpansion

SET cc=clang

REM Get's list of all C files
SET c_filenames= 
FOR /R %%f in (*.c) do (
	SET c_filenames=!c_filenames! %%f
)

SET compiler_flags=-Wall -Wvarargs -Werror -Wno-unused-function -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers
SET include_flags=-Isource -Ithird-party/include
SET linker_flags=-g -Lthird-party/lib -lLLVM-C
SET defines=-D_DEBUG -D_CRT_SECURE_NO_WARNINGS
SET output=-o bin/rift.exe

ECHO "Building rift.exe..."
%cc% %c_filenames% %compiler_flags% %defines% %include_flags% %linker_flags% %output%
