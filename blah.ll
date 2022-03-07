; ModuleID = 'Testing'
source_filename = "Testing"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@0 = private unnamed_addr constant [5 x i8] c"yes\0A\00", align 1

declare i64 @printf(i8*, ...)

define void @test(i1 %0) {
entry:
  %1 = alloca i1, align 1
  store i1 %0, i1* %1, align 1
  %2 = load i1, i1* %1, align 1
  ret void
}

define i64 @main() {
entry:
  call void @test(i1 false)
  %0 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @0, i32 0, i32 0))
  ret i64 0
}
