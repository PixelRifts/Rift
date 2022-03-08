; ModuleID = 'Testing'
source_filename = "Testing"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@k = global i64 0
@0 = private unnamed_addr constant [6 x i8] c"%lld\0A\00", align 1

declare i64 @printf(i8*, ...)

define i1 @gettrue() {
entry:
  ret i1 true
}

define i64 @main() {
entry:
  %i = alloca i64, align 8
  store i64 0, i64* %i, align 8
  br label %cn

cn:                                               ; preds = %wh, %entry
  %0 = load i64, i64* %i, align 8
  %1 = icmp slt i64 %0, 10
  br i1 %1, label %wh, label %af

wh:                                               ; preds = %cn
  %2 = load i64, i64* %i, align 8
  %3 = add i64 %2, 1
  store i64 %3, i64* %i, align 8
  %4 = load i64, i64* %i, align 8
  %5 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([6 x i8], [6 x i8]* @0, i32 0, i32 0), i64 %4)
  br label %cn

af:                                               ; preds = %cn
  ret i64 0
}
