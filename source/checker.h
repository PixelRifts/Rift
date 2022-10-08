/* date = October 8th 2022 4:34 pm */

#ifndef CHECKER_H
#define CHECKER_H

#include "ast_nodes.h"
#include "types.h"

//~ Checker

typedef struct C_Checker {
	IR_Ast* ast;
	b8 errored;
} C_Checker;

void C_Init(C_Checker* checker, IR_Ast* ast);
b8 C_Check(C_Checker* checker);
void C_Free(C_Checker* checker);

#endif //CHECKER_H
