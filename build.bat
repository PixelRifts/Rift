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
if %cc%==gcc SET compiler_flags=-Wall -Wvarargs -Werror -Wno-unused-function -Wno-format-security -Wno-discarded-qualifiers
SET include_flags=-Isource -Ilib/include
SET linker_flags=-g -lshell32
SET defines=-D_DEBUG -DCPLATEST -D_CRT_SECURE_NO_WARNINGS
SET output=-o bin/cpcom.exe

ECHO "Building cpcom.exe..."
%cc% %c_filenames% %compiler_flags% %defines% %include_flags% %linker_flags% %output%