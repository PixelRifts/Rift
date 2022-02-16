#include <stdio.h>
#include <stdlib.h>
#include "str.h"
#include "lexer.h"
#include "parser.h"
#include "checker.h"
#include "defines.h"

#include "ds.h"

Array_Prototype(integer_array, int);
Array_Impl(integer_array, int);

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
        
        integer_array arr = {0};
        integer_array_add(&arr, 10);
        integer_array_add(&arr, 20);
        integer_array_add(&arr, 30);
        
        Array_Iterate(arr, i) {
            printf("%d\n", arr.elems[i]);
        }
        
        L_Lexer lexer = {0};
        L_Init(&lexer, source_str);
        P_Parser parser = {0};
        P_Init(&parser, &lexer);
        AstNode* node = P_Parse(&parser);
        C_Checker checker = {0};
        C_CheckAst(&checker, node);
        //PrintAst(node);
        
        P_Free(&parser);
        
        free(source);
    }
}
