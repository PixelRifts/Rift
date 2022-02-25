; ModuleID = 'Testing'
source_filename = "Testing"

define i64 @main() {
entry:
  %k = alloca i64, align 8
  store i64 7, i64* %k, align 4
}

declare i64 @puts(i8)
