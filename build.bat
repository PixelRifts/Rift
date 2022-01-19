@ECHO off
SetLocal EnableDelayedExpansion

SET cc=clang

del /f generated.c

REM Get's list of all C files
SET c_filenames= 
FOR /R %%f in (*.c) do (
	SET c_filenames=!c_filenames! %%f
)

if %cc%==clang SET compiler_flags=-Wall -Wvarargs -Werror -Wno-unused-function -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers
if %cc%==gcc SET compiler_flags=-Wall -Wvarargs -Werror -Wno-unused-function -Wno-format-security -Wno-discarded-qualifiers -Wno-pointer-to-int-cast
SET include_flags=-Isource -Ithird-party/include
SET linker_flags=-g -lshell32 -Lthird-party/lib -lLLVM-C
SET defines=-D_DEBUG -D_CRT_SECURE_NO_WARNINGS
SET output=-o bin/rift.exe

ECHO "Building rift.exe..."
%cc% %c_filenames% %compiler_flags% %defines% %include_flags% %linker_flags% %output%
