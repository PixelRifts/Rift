#include "utils.h"
#include <stdio.h>
#include <string.h>

string fix_filepath(M_Arena* arena, string filepath) {
    M_Scratch scratch = scratch_get();
    
    string fixed = filepath;
    fixed = str_replace_all(&scratch.arena, fixed, str_lit("\\"), str_lit("/"));
    fixed = str_replace_all(arena, fixed, str_lit("/./"), str_lit("/"));
    while (true) {
        u64 dotdot = str_find_first(fixed, str_lit(".."), 0);
        if (dotdot == fixed.size) break;
        
        u64 last_slash = str_find_last(fixed, str_lit("/"), dotdot - 1);
        
        u64 range = (dotdot + 3) - last_slash;
        string old = fixed;
        fixed = str_alloc(&scratch.arena, fixed.size - range);
        memcpy(fixed.str, old.str, last_slash);
        memcpy(fixed.str + last_slash, old.str + dotdot + 3, old.size - range - last_slash + 1);
    }
    
    scratch_return(&scratch);
    
    return fixed;
}

string full_filepath(M_Arena* arena, string filename) {
    M_Scratch scratch = scratch_get();
    
    char buffer[PATH_MAX];
    get_cwd(buffer, PATH_MAX);
    string cwd = { .str = (u8*) buffer, .size = strlen(buffer) };
    
    string finalized = str_cat(&scratch.arena, cwd, str_lit("/"));
    finalized = str_cat(&scratch.arena, finalized, filename);
    finalized = fix_filepath(arena, finalized);
    
    scratch_return(&scratch);
    return finalized;
}
