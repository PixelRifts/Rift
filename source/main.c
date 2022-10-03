#include <stdio.h>
#include <stdlib.h>
#include "base/str.h"
#include "base/mem.h"
#include "base/utils.h"
#include "lexer.h"

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
    M_ScratchInit();
    
    if (argc < 2) {
        printf("Did not recieve filename as first argument\n");
    } else {
        char* source = readFile(argv[1]);
        string source_str = { .str = (u8*) source, .size = strlen(source) };
        string source_filename = { .str = (u8*) argv[1], .size = strlen(argv[1]) };
        
        L_Lexer lexer = {0};
        L_Init(&lexer, source_str);
        
        free(source);
    }
    
    M_ScratchFree();
}
