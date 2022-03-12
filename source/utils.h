/* date = March 11th 2022 2:07 pm */

#ifndef UTILS_H
#define UTILS_H

#include "mem.h"
#include "str.h"
#include "defines.h"

string fix_filepath(M_Arena* arena, string filepath);
string full_filepath(M_Arena* arena, string filename);
string filename_from_filepath(string filepath);

#endif //UTILS_H
