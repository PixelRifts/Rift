; ModuleID = 'Testing'
source_filename = "Testing"

@0 = private unnamed_addr constant [6 x i8] c"%lld\0A\00", align 1

define i32 @main() {
entry:
  %b = alloca i64, align 8
  store i64 7, i64* %b, align 4
  %0 = load i64, i64* %b, align 4
  %khi = alloca void ()*, align 8
  store void ()* @f, void ()** %khi, align 8
  %1 = load void ()*, void ()** %khi, align 8
  %lambda = alloca void ()*, align 8
  store void ()* @f.1, void ()** %lambda, align 8
  %2 = load void ()*, void ()** %lambda, align 8
  %m = alloca i64, align 8
  store i64 10, i64* %m, align 4
  %3 = load i64, i64* %m, align 4
  %4 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([6 x i8], [6 x i8]* @0, i32 0, i32 0), i64 %3)
  ret i32 0
}

declare i64 @printf(i8*, ...)

define void @f() {
entry:
  %k = alloca i64, align 8
  store i64 20, i64* %k, align 4
  %0 = load i64, i64* %k, align 4
  ret void
}

define void @f.1() {
entry:
  %k = alloca i64, align 8
  store i64 30, i64* %k, align 4
  %0 = load i64, i64* %k, align 4
  ret void
}
