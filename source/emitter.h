/* date = October 12th 2021 8:07 pm */

#ifndef EMITTER_H
#define EMITTER_H

#include "lexer.h"
#include "parser.h"
#include <stdio.h>

typedef struct E_Emitter {
    P_Parser parser;
    FILE* output_file;
} E_Emitter;

void E_Initialize(E_Emitter* emitter, string source, string filename);
void E_Emit(E_Emitter* emitter);
void E_Free(E_Emitter* emitter);

#endif //EMITTER_H
