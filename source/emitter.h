/* date = October 12th 2021 8:07 pm */

#ifndef EMITTER_H
#define EMITTER_H

#include "lexer.h"
#include "parser.h"
#include <stdio.h>

typedef struct E_Emitter {
    P_Parser parser;
    u64 line;
    
    FILE* output_file;
} E_Emitter;

void E_Initialize(E_Emitter* emitter, string source);
void E_Emit(E_Emitter* emitter);
void E_Free(E_Emitter* emitter);

#endif //EMITTER_H
