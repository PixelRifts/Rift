/* date = October 8th 2022 4:51 pm */

#ifndef TYPES_H
#define TYPES_H

//~

typedef struct Type Type;

typedef u32 TypeKind;
enum {
	TypeKind_Invalid,
	TypeKind_Regular,
	TypeKind_COUNT,
};

typedef u32 RegularTypeKind;
enum {
	RegularTypeKind_Invalid,
	RegularTypeKind_Integer,
	RegularTypeKind_COUNT,
};


struct Type {
	TypeKind kind;
	union {
		RegularTypeKind regular;
	};
};

typedef u64 TypeID;

#endif //TYPES_H
