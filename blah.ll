; ModuleID = 'Testing'
source_filename = "Testing"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@k = global i64 9
@0 = private unnamed_addr constant [6 x i8] c"yes\\n\00", align 1

declare i64 @printf(i8*, ...)

define i64 @add_ten(i64 %0) {
entry:
  %1 = alloca i64, align 8
  store i64 %0, i64* %1, align 8
  %2 = load i64, i64* %1, align 8
  %3 = add i64 10, %2
  ret i64 %3
}

define i64 @variadic(i8* %0, ...) {
entry:
  %1 = alloca i8*, align 8
  store i8* %0, i8** %1, align 8
  %2 = load i8*, i8** %1, align 8
  ret i64 10
}

define i64 @main() {
entry:
  %l = alloca i64, align 8
  %0 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([6 x i8], [6 x i8]* @0, i32 0, i32 0), i64 6)
  %1 = load i64, i64* %l, align 8
  ret i64 0
}
