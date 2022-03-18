; ModuleID = 'Testing'
source_filename = "Testing"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@try = global i64 10, !dbg !0

declare i64 @printf(i8*, ...)

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

define i64 @main() {
entry:
  %0 = alloca i64, align 8
  %i = alloca i64, align 8, !dbg !11
  call void @llvm.dbg.declare(metadata i64* %i, metadata !8, metadata !DIExpression()), !dbg !11
  store i64 0, i64* %i, align 8, !dbg !12
  %j = alloca i64, align 8, !dbg !12
  call void @llvm.dbg.declare(metadata i64* %j, metadata !13, metadata !DIExpression()), !dbg !14
  store i64 10, i64* %j, align 8, !dbg !12
  %k = alloca i64, align 8, !dbg !12
  call void @llvm.dbg.declare(metadata i64* %k, metadata !15, metadata !DIExpression()), !dbg !16
  store i64 20, i64* %k, align 8, !dbg !12
  store i64 0, i64* %0, align 8, !dbg !12
  br label %re, !dbg !12

re:                                               ; preds = %entry
  %1 = load i64, i64* %0, align 8, !dbg !12
  ret i64 %1, !dbg !12
}

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!4}
!llvm.dbg.cu = !{!5}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "try", scope: !2, file: !2, line: 1, type: !3, isLocal: true, isDefinition: true, align: 8)
!2 = !DIFile(filename: "test.rf", directory: "P:/C/cp")
!3 = !DIBasicType(name: "Integer", size: 64, encoding: DW_ATE_signed)
!4 = !{i32 1, !"Debug Info Version", i32 3}
!5 = distinct !DICompileUnit(language: DW_LANG_C, file: !2, producer: "rift", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !6, globals: !7, splitDebugInlining: false)
!6 = !{}
!7 = !{!0}
!8 = !DILocalVariable(name: "i", scope: !9, file: !2, line: 4, type: !3, align: 8)
!9 = distinct !DISubprogram(name: "main", linkageName: "main", scope: !2, file: !2, line: 3, type: !10, scopeLine: 3, flags: DIFlagStaticMember, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !5, retainedNodes: !6)
!10 = !DISubroutineType(flags: DIFlagStaticMember, types: !6)
!11 = !DILocation(line: 4, scope: !9)
!12 = !DILocation(line: 1, scope: !9)
!13 = !DILocalVariable(name: "j", scope: !9, file: !2, line: 5, type: !3, align: 8)
!14 = !DILocation(line: 5, scope: !9)
!15 = !DILocalVariable(name: "k", scope: !9, file: !2, line: 6, type: !3, align: 8)
!16 = !DILocation(line: 6, scope: !9)
