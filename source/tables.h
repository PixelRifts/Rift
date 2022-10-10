/* date = October 9th 2022 9:33 pm */

#ifndef TABLES_H
#define TABLES_H

// NOTE(voxel): All operator tables are 0 terminated

typedef struct TypePair {
	TypeID a;
	TypeID r;
} TypePair;

typedef struct TypeTriple {
	TypeID a;
	TypeID b;
	TypeID r;
} TypeTriple;

TypePair unary_operator_table_plusminus[] = {
	{ TypeID_Integer, TypeID_Integer },
	
	{ TypeID_Invalid, TypeID_Invalid },
};

TypeTriple binary_operator_table_plusminus[] = {
	{ TypeID_Integer, TypeID_Integer, TypeID_Integer },
	
	{ TypeID_Invalid, TypeID_Invalid, TypeID_Invalid },
};

TypeTriple binary_operator_table_stardivmod[] = {
	{ TypeID_Integer, TypeID_Integer, TypeID_Integer },
	
	{ TypeID_Invalid, TypeID_Invalid, TypeID_Invalid },
};

#endif //TABLES_H
