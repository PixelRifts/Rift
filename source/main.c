#include <stdio.h>
#include <stdlib.h>
#include "str.h"
#include "lexer.h"
#include "parser.h"
#include "checker.h"
#include "llvm_backend.h"
#include "defines.h"

#include "llvm-c/Core.h"

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    
    char* buffer = (char*)malloc(fileSize + 1);
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';
    
    fclose(file);
    return buffer;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Did not recieve filename as first argument\n");
    } else {
        char* source = readFile(argv[1]);
        string source_str = { .str = (u8*) source, .size = strlen(source) };
        
        L_Lexer lexer = {0};
        L_Init(&lexer, source_str);
        P_Parser parser = {0};
        P_Init(&parser, &lexer);
        AstNode* node = P_Parse(&parser);
        PrintAst(node);
        fflush(stdout);
        C_Checker checker = {0};
        C_Init(&checker);
        C_CheckAst(&checker, node);
        BL_Emitter emitter = {0};
        BL_Init(&emitter, str_lit("blah.ll"));
        BL_Emit(&emitter, node);
        BL_Free(&emitter);
        C_Free(&checker);
        P_Free(&parser);
        
        free(source);
    }
}

/* Hello World */
/*
LLVMContextRef context = LLVMContextCreate();
        LLVMModuleRef  module = LLVMModuleCreateWithNameInContext("hello", context);
        LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
        
        // types
        LLVMTypeRef int_8_type = LLVMInt8TypeInContext(context);
        LLVMTypeRef int_8_type_ptr = LLVMPointerType(int_8_type, 0);
        LLVMTypeRef int_32_type = LLVMInt32TypeInContext(context);
        
        // puts function
        LLVMTypeRef puts_function_args_type[] = {
            int_8_type_ptr
        };
        
        LLVMTypeRef  puts_function_type = LLVMFunctionType(int_32_type, puts_function_args_type, 1, false);
        LLVMValueRef puts_function = LLVMAddFunction(module, "puts", puts_function_type);
        // end
        
        // main function
        LLVMTypeRef  main_function_type = LLVMFunctionType(int_32_type, nullptr, 0, false);
        LLVMValueRef main_function = LLVMAddFunction(module, "main", main_function_type);
        
        LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, main_function, "entry");
        LLVMPositionBuilderAtEnd(builder, entry);
        
        LLVMValueRef puts_function_args[] = {
            LLVMBuildPointerCast(builder, // cast [14 x i8] type to int8 pointer
                                 LLVMBuildGlobalString(builder, "Hello, World!", "hello"), // build hello string constant
                                 int_8_type_ptr, "0")
        };
        
        LLVMBuildCall2(builder, puts_function_type, puts_function, puts_function_args, 1, "i");
        LLVMBuildRet(builder, LLVMConstInt(int_32_type, 0, false));
        // end
        
        //LLVMDumpModule(module); // dump module to STDOUT
        LLVMPrintModuleToFile(module, "hello.ll", nullptr);
        
        // clean memory
        LLVMDisposeBuilder(builder);
        LLVMDisposeModule(module);
        LLVMContextDispose(context);
*/
