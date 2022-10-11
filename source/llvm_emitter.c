#include "llvm_emitter.h"

#include "llvm-c/Analysis.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Error.h"

//~ Helpers

static void LLVM_Report_LLVMHandler(const char* reason) { fprintf(stderr, "LLVM Error %s", reason); }

//~ Code emission

static LLVMValueRef LLVM_EmitUnary(LLVM_Emitter* emitter, L_TokenType op, LLVMValueRef operand) {
	switch (op) {
		case TokenType_Plus: return operand;
		case TokenType_Minus: return LLVMBuildNeg(emitter->builder, operand, "");
		
		default: unreachable;
	}
	
	return (LLVMValueRef) {0};
}

static LLVMValueRef LLVM_EmitBinary(LLVM_Emitter* emitter, L_TokenType op, LLVMValueRef a, LLVMValueRef b) {
	switch (op) {
		case TokenType_Plus: return LLVMBuildAdd(emitter->builder, a, b, "");
		case TokenType_Minus: return LLVMBuildSub(emitter->builder, a, b, "");
		case TokenType_Star: return LLVMBuildMul(emitter->builder, a, b, "");
		case TokenType_Slash: return LLVMBuildUDiv(emitter->builder, a, b, "");
		case TokenType_Percent: return LLVMBuildURem(emitter->builder, a, b, "");
		
		default: unreachable;
	}
	return (LLVMValueRef) {0};
}


LLVMValueRef LLVM_Emit(LLVM_Emitter* emitter, IR_Ast* ast) {
	switch (ast->type) {
		case AstType_IntLiteral: {
			return LLVMConstInt(emitter->int_32_type, ast->int_lit.value, false);
		} break;
		
		case AstType_FloatLiteral: {
			// TODO(voxel): 
		} break;
		
		case AstType_ExprUnary: {
			LLVMValueRef operand = LLVM_Emit(emitter, ast->unary.operand);
			return LLVM_EmitUnary(emitter, ast->unary.operator.type, operand);
		} break;
		
		case AstType_ExprBinary: {
			LLVMValueRef a = LLVM_Emit(emitter, ast->binary.a);
			LLVMValueRef b = LLVM_Emit(emitter, ast->binary.b);
			return LLVM_EmitBinary(emitter, ast->binary.operator.type, a, b);
		} break;
		
		case AstType_StmtPrint: {
			// NOTE(voxel): Very Hardcoded ATM
			
			LLVMValueRef args[2] = {
				LLVMBuildPointerCast(emitter->builder, // cast [14 x i8] type to int8 pointer
									 LLVMBuildGlobalString(emitter->builder, "%d", ""),
									 emitter->int_8_type_ptr, ""),
				LLVM_Emit(emitter, ast->print.value),
			};
			
			return LLVMBuildCall2(emitter->builder, emitter->printf_type, emitter->printf_object, args, 2, "");
		} break;
	}
	
	return (LLVMValueRef) {0};
}

//~ Init/Free

#define INITIALIZE_TARGET(X) do { \
LLVMInitialize ## X ## AsmParser(); \
LLVMInitialize ## X ## AsmPrinter(); \
LLVMInitialize ## X ## TargetInfo(); \
LLVMInitialize ## X ## Target(); \
LLVMInitialize ## X ## Disassembler(); \
LLVMInitialize ## X ## TargetMC(); \
} while(0)

void LLVM_Init(LLVM_Emitter* emitter) {
	MemoryZeroStruct(emitter, LLVM_Emitter);
	
	INITIALIZE_TARGET(X86);
	
	emitter->context = LLVMContextCreate();
	emitter->module = LLVMModuleCreateWithNameInContext("Testing", emitter->context);
	emitter->builder = LLVMCreateBuilderInContext(emitter->context);
	
	LLVMTargetRef target;
	char* error = nullptr;
	if (LLVMGetTargetFromTriple(LLVMGetDefaultTargetTriple(), &target, &error) != 0) {
		printf("Target From Triple: %s\n", error);
		flush;
	}
	LLVMSetTarget(emitter->module, LLVMGetDefaultTargetTriple());
	LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(target, LLVMGetDefaultTargetTriple(), "", "", LLVMCodeGenLevelNone, LLVMRelocDefault, LLVMCodeModelDefault);
	LLVMTargetDataRef target_data = LLVMCreateTargetDataLayout(target_machine);
	LLVMSetModuleDataLayout(emitter->module, target_data);
	
	LLVMInstallFatalErrorHandler(LLVM_Report_LLVMHandler);
	LLVMEnablePrettyStackTrace();
	
	emitter->int_8_type = LLVMInt8TypeInContext(emitter->context);
	emitter->int_8_type_ptr = LLVMPointerType(emitter->int_8_type, 0);
	emitter->int_32_type = LLVMInt32TypeInContext(emitter->context);
	
	LLVMTypeRef printf_params[] = { emitter->int_8_type_ptr };
	emitter->printf_type = LLVMFunctionType(emitter->int_32_type, printf_params, 1, true);
	emitter->printf_object = LLVMAddFunction(emitter->module, "printf", emitter->printf_type);
	
	LLVMTypeRef main_function_type = LLVMFunctionType(emitter->int_32_type, nullptr, 0, false);
	LLVMValueRef main_function = LLVMAddFunction(emitter->module, "main", main_function_type);
	
	LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(emitter->context, main_function, "entry");
	LLVMPositionBuilderAtEnd(emitter->builder, entry);
}

void LLVM_Free(LLVM_Emitter* emitter) {
	LLVMBuildRet(emitter->builder, LLVMConstInt(emitter->int_32_type, 0, false));
	
	char* error = nullptr;
	
	LLVMVerifyModule(emitter->module, LLVMPrintMessageAction, nullptr);
	
	LLVMPrintModuleToFile(emitter->module, "hello.ll", &error);
	if (error) {
		fprintf(stderr, "Print Module Error: %s\n", error);
		fflush(stderr);
		LLVMDisposeErrorMessage(error);
	}
	
	LLVMDisposeBuilder(emitter->builder);
	LLVMDisposeModule(emitter->module);
	LLVMContextDispose(emitter->context);
}
