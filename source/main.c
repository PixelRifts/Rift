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
        C_CheckAst(&checker, node);
        BL_Emitter emitter = {0};
        BL_Init(&emitter, str_lit("blah.ll"));
        BL_Emit(&emitter, node);
        BL_Free(&emitter);
        P_Free(&parser);
        
        free(source);
    }
}
