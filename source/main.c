#include <stdio.h>
#include <stdlib.h>
#include "defines.h"
#include "lexer.h"
#include "parser.h"
#include "emitter.h"

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
        printf("yo gimme a file\n");
    } else {
        M_Arena scratch = {0};
        arena_init(&scratch);
        
        // TODO(voxel): move source to scratch
        char* source = readFile(argv[1]);
        string filename = (string) { .str = (u8*)argv[1], .size = strlen(argv[1]) };
        string_list tags = {0};
        if (argc > 2) {
            for (u32 i = 2; i < argc; i++) {
                string tagname = (string) { .str = ((u8*)argv[i]) + 1, .size = strlen(argv[i]) - 1 };
                string_list_push(&scratch, &tags, tagname);
            }
        }
        
#ifdef CPCOM_WIN
        string_list_push(&scratch, &tags, str_lit("windows"));
#elif defined(CPCOM_LINUX)
        string_list_push(&scratch, &tags, str_lit("linux"));
#endif
        
        E_Emitter emitter = {0};
        E_Initialize(&emitter, (string) { .str = (u8*)source, .size = strlen(source) }, filename, tags);
        E_Emit(&emitter);
        E_Free(&emitter);
        
        arena_free(&scratch);
        free(source);
    }
}
