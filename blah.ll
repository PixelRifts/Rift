; ModuleID = 'Testing'
source_filename = "Testing"

@0 = private unnamed_addr constant [6 x i8] c"%lld\0A\00", align 1

define i32 @main() {
entry:
  %m = alloca i64, align 8
  store i64 10, i64* %m, align 4
  %0 = load i64, i64* %m, align 4
  %1 = call i64 (i8*, ...) @printf(i8* getelementptr inbounds ([6 x i8], [6 x i8]* @0, i32 0, i32 0), i64 %0)
  ret i32 0
}

declare i64 @printf(i8*, ...)
