/* date = October 27th 2021 7:29 pm */

#ifndef TYPES_H
#define TYPES_H

typedef string P_ValueType;

static const string ValueType_Invalid = str_lit("INVALID");

static const string ValueType_Integer = str_lit("int");
static const string ValueType_Long = str_lit("long");
static const string ValueType_Float = str_lit("float");
static const string ValueType_Double = str_lit("double");

static const string ValueType_String = str_lit("string");
static const string ValueType_Char = str_lit("char");
static const string ValueType_Bool = str_lit("bool");
static const string ValueType_Void = str_lit("void");
static const string ValueType_Tombstone = str_lit("tombstone");

b8 type_check(P_ValueType a, P_ValueType expected);
b8 node_type_check(string_list_node* a, string_list_node* expected);

#endif //TYPES_H
