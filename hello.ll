; ModuleID = 'hello'
source_filename = "hello"

@hello = private unnamed_addr constant [14 x i8] c"Hello, World!\00", align 1

declare i32 @puts(i8*)

define i32 @main() {
entry:
  %i = call i32 @puts(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @hello, i32 0, i32 0))
  ret i32 0
}
