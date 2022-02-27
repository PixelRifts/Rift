; ModuleID = 'Testing'
source_filename = "Testing"

@hello = private unnamed_addr constant [5 x i8] c"%lld\00", align 1

define i32 @main() {
entry:
  %b = alloca i64, align 8
  store i64 7, i64* %b, align 4
  %0 = load i64, i64* %b, align 4
  %m = alloca i64, align 8
  %1 = mul i64 2, %0
  %2 = add i64 %0, %1
  store i64 %2, i64* %m, align 4
  %3 = load i64, i64* %m, align 4
  %4 = load i64, i64* %m, align 4
  %5 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @hello, i32 0, i32 0), i64 %4)
  ret i32 0
}

declare i64 @printf(i8*, ...)
