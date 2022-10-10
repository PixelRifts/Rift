/* date = October 8th 2022 4:34 pm */

#ifndef CHECKER_H
#define CHECKER_H

#include "ast_nodes.h"
#include "types.h"
#include "base/ds.h"

DArray_Prototype(Type);

#if 0
typedef struct TypeCache TypeCache;
#endif
typedef darray(Type) TypeCache;


//~ Type Cache

void TypeCache_Init(TypeCache* cache);
TypeID TypeCache_Register(TypeCache* cache, Type type);
Type* TypeCache_Get(TypeCache* cache, TypeID id);
void TypeCache_Free(TypeCache* cache);

enum {
	TypeID_Invalid = 0,
	TypeID_Integer,
	TypeID_COUNT,
};

//~ Checker

typedef struct C_Checker {
	IR_Ast* ast;
	b8 errored;
	
	TypeCache type_cache;
} C_Checker;

void C_Init(C_Checker* checker, IR_Ast* ast);
b8 C_Check(C_Checker* checker);
void C_Free(C_Checker* checker);

#endif //CHECKER_H
