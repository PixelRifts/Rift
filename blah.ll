; ModuleID = 'Testing'
source_filename = "Testing"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@0 = private unnamed_addr constant [5 x i8] c"yes\0A\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"no\0A\00", align 1

declare i64 @printf(i8*, ...)

define i1 @gettrue() {
entry:
  ret i1 true
}

define i64 @main() {
entry:
  %0 = call i1 @gettrue()
  %1 = icmp eq i1 %0, false
  br i1 %1, label %th, label %el

th:                                               ; preds = %entry
  %2 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @0, i32 0, i32 0))
  br label %me

el:                                               ; preds = %entry
  %3 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
  br label %me

me:                                               ; preds = %el, %th
  ret i64 0
}
