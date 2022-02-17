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

Stack_Prototype(integer_stack, int);
Stack_Impl(integer_stack, int);

HashTable_Prototype(int_string, string, int);
b8 intstring_key_isnull(int_string_hash_table_key k) {
    return k.size == 0;
}
b8 intstring_val_isnull(int_string_hash_table_value v) {
    return v == 0;
}
b8 intstring_val_istomb(int_string_hash_table_value v) {
    return v == -69;
}
HashTable_Impl(int_string, intstring_key_isnull, str_eq, str_hash, -69, intstring_val_isnull, intstring_val_istomb);

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
        
        printf("Array:\n");
        integer_array arr = {0};
        integer_array_add(&arr, 10);
        integer_array_add(&arr, 20);
        integer_array_add(&arr, 30);
        int val = integer_array_remove(&arr, 1);
        printf("%d\n", val);
        Iterate(arr, i) {
            printf("%d\n", arr.elems[i]);
        }
        
        printf("\nStack:\n");
        integer_stack stack = {0};
        integer_stack_push(&stack, 40);
        integer_stack_push(&stack, 50);
        integer_stack_push(&stack, 60);
        val = integer_stack_pop(&stack);
        printf("%d\n", val);
        Iterate(stack, i) {
            printf("%d\n", stack.elems[i]);
        }
        
        printf("\nHash Table:\n");
        int_string_hash_table table = {0};
        int_string_hash_table_set(&table, str_lit("YOLO"), 40);
        int_string_hash_table_set(&table, str_lit("BOO"), 20);
        int_string_hash_table_set(&table, str_lit("KEK"), 30);
        int_string_hash_table_del(&table, str_lit("YOLO"));
        
        int boo;
        int_string_hash_table_get(&table, str_lit("BOO"), &boo);
        printf("%d\n", boo);
        int_string_hash_table_get(&table, str_lit("KEK"), &boo);
        printf("%d\n", boo);
        
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
