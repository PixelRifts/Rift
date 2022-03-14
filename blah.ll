; ModuleID = 'Testing'
source_filename = "Testing"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@0 = private unnamed_addr constant [6 x i8] c"%lld\0A\00", align 1

declare i64 @printf(i8*, ...)

define i64 @add(i64 %0, i64 %1) {
entry:
  %2 = alloca i64, align 8
  %3 = alloca i64, align 8
  store i64 %0, i64* %3, align 8
  %4 = alloca i64, align 8
  store i64 %1, i64* %4, align 8
  %5 = load i64, i64* %3, align 8
  %6 = load i64, i64* %4, align 8
  %7 = add i64 %5, %6
  store i64 %7, i64* %2, align 8
  br label %re

re:                                               ; preds = %entry
  %8 = load i64, i64* %2, align 8
  ret i64 %8
}

define i64 @main() {
entry:
  %0 = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 0, i64* %i, align 8
  br label %cn

re:                                               ; preds = %af
  %1 = load i64, i64* %0, align 8
  ret i64 %1

cn:                                               ; preds = %wh, %entry
  %2 = load i64, i64* %i, align 8
  %3 = icmp slt i64 %2, 10
  br i1 %3, label %wh, label %af

wh:                                               ; preds = %cn
  %4 = load i64, i64* %i, align 8
  %5 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([6 x i8], [6 x i8]* @0, i32 0, i32 0), i64 %4)
  %6 = load i64, i64* %i, align 8
  %7 = add i64 %6, 1
  store i64 %7, i64* %i, align 8
  br label %cn

af:                                               ; preds = %cn
  store i64 0, i64* %0, align 8
  br label %re
}

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 1, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C, file: !2, producer: "rift", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false)
!2 = !DIFile(filename: "test.rf", directory: "P:/C/cp")
