; ModuleID = 'Testing'
source_filename = "Testing"

@hello = private unnamed_addr constant [5 x i8] c"%lld\00", align 1

define i32 @main() {
entry:
  %b = alloca i64, align 8
  store i64 7, i64* %b, align 4
  %m = alloca i64, align 8
  %0 = load i64, i64* %b, align 4
  %1 = add i64 %0, 6
  store i64 %1, i64* %m, align 4
  %2 = load i64, i64* %m, align 4
  %3 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @hello, i32 0, i32 0), i64 %2)
  ret i32 0
}

declare i64 @printf(i8*, ...)
