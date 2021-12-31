#include "emitter.h"
#include <stdarg.h>

//#define OUT_TO_CONSOLE

// All types will go through this map
static P_ValueType type_map(E_Emitter* emitter, P_ValueType type) {
    type.full_type = str_replace_all(&emitter->parser.arena, type.full_type, str_lit("long"), str_lit("long long"));
    type.full_type = str_replace_all(&emitter->parser.arena, type.full_type, str_lit("string"), str_lit("const char *"));
    return type;
}

static void E_Write(E_Emitter* emitter, const char* text) {
#ifdef OUT_TO_CONSOLE
    printf(text);
#else
    fprintf(emitter->output_file, text);
#endif
}

static void E_WriteLine(E_Emitter* emitter, const char* text) {
#ifdef OUT_TO_CONSOLE
    printf(text);
    printf("\n");
#else
    fprintf(emitter->output_file, text);
    fprintf(emitter->output_file, "\n");
#endif
}

static void E_WriteF(E_Emitter* emitter, const char* text, ...) {
    va_list args;
    va_start(args, text);
#ifdef OUT_TO_CONSOLE
    vprintf(text, args);
#else
    vfprintf(emitter->output_file, text, args);
#endif
    va_end(args);
}

static void E_WriteLineF(E_Emitter* emitter, const char* text, ...) {
    va_list args;
    va_start(args, text);
#ifdef OUT_TO_CONSOLE
    vprintf(text, args);
    printf("\n");
#else
    vfprintf(emitter->output_file, text, args);
    fprintf(emitter->output_file, "\n");
#endif
    va_end(args);
}

static void E_EmitExpression(E_Emitter* emitter, P_Expr* expr);
static void E_EmitTypeAndName(E_Emitter* emitter, P_ValueType* type, string name, b8 noname);

static void E_EmitTypeMods_Recursive(E_Emitter* emitter, P_ValueTypeMod* mods, i32 mod_idx, string name_if_negone, b8 is_only_one) {
    if (mod_idx == -1) {
        E_WriteF(emitter, "%.*s", str_expand(name_if_negone));
        return;
    }
    
    if (!is_only_one)
        E_Write(emitter, "(");
    switch (mods[mod_idx].type) {
        case ValueTypeModType_Pointer:
        case ValueTypeModType_Reference: {
            E_Write(emitter, "*");
            E_EmitTypeMods_Recursive(emitter, mods, mod_idx - 1, name_if_negone, false);
        } break;
        case ValueTypeModType_Array: {
            E_EmitTypeMods_Recursive(emitter, mods, mod_idx - 1, name_if_negone, false);
            E_Write(emitter, "[");
            E_EmitExpression(emitter, mods[mod_idx].op.array_expr);
            E_Write(emitter, "]");
        } break;
    }
    if (!is_only_one)
        E_Write(emitter, ")");
}

static P_ValueType* E_GetBottommostReturntype(P_ValueType* type) {
    if (type->type == ValueTypeType_FuncPointer)
        return E_GetBottommostReturntype(type->op.func_ptr.ret_type);
    else return type;
}

static void E_EmitTypeFnptr(E_Emitter* emitter, P_ValueType* type, string name) {
    if (type->type == ValueTypeType_FuncPointer) {
        E_Write(emitter, "(*");
        E_EmitTypeFnptr(emitter, type->op.func_ptr.ret_type, name);
        E_Write(emitter, "(");
        value_type_list_node* curr = type->op.func_ptr.func_param_types->first;
        while (curr != nullptr) {
            E_EmitTypeAndName(emitter, &curr->type, (string){0}, false);
            if (curr->next != nullptr)
                E_Write(emitter, ", ");
            curr = curr->next;
        }
        E_Write(emitter, "))");
    } else {
        E_WriteF(emitter, "(*%.*s)", str_expand(name));
    }
}

static void E_EmitTypeAndName(E_Emitter* emitter, P_ValueType* type, string name, b8 noname) {
    if (type->type == ValueTypeType_FuncPointer) {
        P_ValueType* ret = E_GetBottommostReturntype(type);
        E_EmitTypeAndName(emitter, ret, (string){0}, true);
        E_EmitTypeFnptr(emitter, type->op.func_ptr.ret_type, name);
        E_Write(emitter, "(");
        value_type_list_node* curr = type->op.func_ptr.func_param_types->first;
        while (curr != nullptr) {
            E_EmitTypeAndName(emitter, &curr->type, (string){0}, false);
            if (curr->next != nullptr)
                E_Write(emitter, ", ");
            curr = curr->next;
        }
        E_Write(emitter, ")");
        
    } else {
        E_WriteF(emitter, "%.*s ", str_expand(type->base_type));
        if (type->mod_ct != 0) {
            E_EmitTypeMods_Recursive(emitter, type->mods, type->mod_ct - 1, name, type->mod_ct == 1);
        } else {
            E_WriteF(emitter, "%.*s", str_expand(name));
        }
    }
}

static void E_EmitTypeFnptr_Fndecl(E_Emitter* emitter, P_ValueType* type, string name, b8 noname, string_list param_names, value_type_list param_types, u32 arity, b8 varargs, string* varargs_name, string* arg_before_varargs_name) {
    if (type->type == ValueTypeType_FuncPointer) {
        E_Write(emitter, "(*");
        E_EmitTypeFnptr_Fndecl(emitter, type->op.func_ptr.ret_type, name, true, param_names, param_types, arity, varargs, varargs_name, arg_before_varargs_name);
        E_Write(emitter, "(");
        value_type_list_node* curr = type->op.func_ptr.func_param_types->first;
        while (curr != nullptr) {
            E_EmitTypeAndName(emitter, &curr->type, (string){0}, false);
            if (curr->next != nullptr)
                E_Write(emitter, ", ");
            curr = curr->next;
        }
        E_Write(emitter, "))");
    } else {
        E_WriteF(emitter, "(*%.*s(", str_expand(name));
        string_list_node* curr_name = param_names.first;
        value_type_list_node* curr_type = param_types.first;
        if (arity != 0) {
            for (u32 i = 0; i < arity; i++) {
                P_ValueType c_type = type_map(emitter, curr_type->type);
                if (str_eq(c_type.full_type, str_lit("..."))) {
                    E_WriteF(emitter, "%.*s", str_expand(c_type.full_type));
                    *varargs_name = (string) { .str = curr_name->str, .size = curr_name->size };
                    break;
                }
                
                E_EmitTypeAndName(emitter, &c_type, (string) { .str = curr_name->str, .size = curr_name->size }, false);
                if (i != arity - 1)
                    E_Write(emitter, ", ");
                
                if (varargs) {
                    if (i == arity - 2)
                        *arg_before_varargs_name = (string) { .str = curr_name->str, .size = curr_name->size };
                }
                
                curr_name = curr_name->next;
                curr_type = curr_type->next;
            }
        }
        if (arity == 0)
            E_Write(emitter, "void");
        E_Write(emitter, "))");
    }
}

static void E_EmitTypeAndName_Fndecl(E_Emitter* emitter, P_ValueType* type, string name, b8 noname, string_list param_names, value_type_list param_types, u32 arity, b8 varargs, string* varargs_name, string* arg_before_varargs_name) {
    if (type->type == ValueTypeType_FuncPointer) {
        
        P_ValueType* ret = E_GetBottommostReturntype(type);
        E_EmitTypeAndName(emitter, ret, (string){0}, false);
        E_EmitTypeFnptr_Fndecl(emitter, type->op.func_ptr.ret_type, name, true, param_names, param_types, arity, varargs, varargs_name, arg_before_varargs_name);
        E_Write(emitter, "(");
        value_type_list_node* curr = type->op.func_ptr.func_param_types->first;
        while (curr != nullptr) {
            E_EmitTypeAndName(emitter, &curr->type, (string){0}, false);
            if (curr->next != nullptr)
                E_Write(emitter, ", ");
            curr = curr->next;
        }
        E_Write(emitter, ")");
        
    } else {
        E_EmitTypeAndName(emitter, type, name, noname);
        E_Write(emitter, "(");
        
        string_list_node* curr_name = param_names.first;
        value_type_list_node* curr_type = param_types.first;
        if (arity != 0) {
            for (u32 i = 0; i < arity; i++) {
                P_ValueType c_type = type_map(emitter, curr_type->type);
                if (str_eq(c_type.full_type, str_lit("..."))) {
                    E_WriteF(emitter, "%.*s", str_expand(c_type.full_type));
                    *varargs_name = (string) { .str = curr_name->str, .size = curr_name->size };
                    break;
                }
                
                E_EmitTypeAndName(emitter, &c_type, (string) { .str = curr_name->str, .size = curr_name->size }, false);
                if (i != arity - 1)
                    E_Write(emitter, ", ");
                
                if (varargs) {
                    if (i == arity - 2)
                        *arg_before_varargs_name = (string) { .str = curr_name->str, .size = curr_name->size };
                }
                
                curr_name = curr_name->next;
                curr_type = curr_type->next;
            }
        }
        if (arity == 0)
            E_Write(emitter, "void");
        E_Write(emitter, ")");
    }
}

static void E_BeginEmitting(E_Emitter* emitter) {
    E_WriteLine(emitter, "#include <stdio.h>");
    E_WriteLine(emitter, "#include <stdbool.h>");
    E_WriteLine(emitter, "#include <stdarg.h>");
    E_WriteLine(emitter, "#include <stdlib.h>");
    E_WriteLine(emitter, "#include <string.h>");
    E_WriteLine(emitter, "");
}

static void E_FinishEmitting(E_Emitter* emitter) {}

static void E_EmitStatementChain(E_Emitter* emitter, P_Stmt* stmts, u32 indent);

static void E_EmitExpression(E_Emitter* emitter, P_Expr* expr) {
    switch (expr->type) {
        case ExprType_IntLit: {
            E_WriteF(emitter, "%d", expr->op.integer_lit);
        } break;
        
        case ExprType_LongLit: {
            E_WriteF(emitter, "%I64d", expr->op.long_lit);
        } break;
        
        case ExprType_FloatLit: {
            E_WriteF(emitter, "%f", expr->op.float_lit);
        } break;
        
        case ExprType_DoubleLit: {
            E_WriteF(emitter, "%f", expr->op.double_lit);
        } break;
        
        case ExprType_BoolLit: {
            E_WriteF(emitter, "%s", expr->op.bool_lit ? "true" : "false");
        } break;
        
        case ExprType_CharLit: {
            E_WriteF(emitter, "%.*s", str_expand(expr->op.char_lit));
        } break;
        
        case ExprType_StringLit: {
            E_WriteF(emitter, "%.*s", str_expand(expr->op.string_lit));
        } break;
        
        case ExprType_ArrayLit: {
            E_Write(emitter, "{ ");
            for (u32 i = 0; i < expr->op.array.count; i++) {
                E_EmitExpression(emitter, expr->op.array.elements[i]);
                E_Write(emitter, ", ");
            }
            E_Write(emitter, " }");
        } break;
        
        case ExprType_Assignment: {
            E_EmitExpression(emitter, expr->op.assignment.name);
            E_Write(emitter, " = ");
            E_EmitExpression(emitter, expr->op.assignment.value);
        } break;
        
        case ExprType_Binary: {
            E_Write(emitter, "(");
            E_EmitExpression(emitter, expr->op.binary.left);
            E_WriteF(emitter, "%s", L__get_string_from_type__(expr->op.binary.operator).str);
            E_EmitExpression(emitter, expr->op.binary.right);
            E_Write(emitter, ")");
        } break;
        
        case ExprType_Unary: {
            E_Write(emitter, "(");
            E_WriteF(emitter, "%s", L__get_string_from_type__(expr->op.unary.operator).str);
            E_EmitExpression(emitter, expr->op.unary.operand);
            E_Write(emitter, ")");
        } break;
        
        case ExprType_Variable: {
            E_WriteF(emitter, "%.*s", str_expand(expr->op.variable));
        } break;
        
        case ExprType_FuncCall: {
            E_WriteF(emitter, "%.*s(", str_expand(expr->op.func_call.name));
            for (u32 i = 0; i < expr->op.func_call.call_arity; i++) {
                E_EmitExpression(emitter, expr->op.func_call.params[i]);
                if (i != expr->op.func_call.call_arity - 1)
                    E_Write(emitter, ", ");
            }
            E_Write(emitter, ")");
        } break;
        
        case ExprType_Call: {
            E_EmitExpression(emitter, expr->op.call.left);
            E_WriteF(emitter, "(");
            for (u32 i = 0; i < expr->op.call.call_arity; i++) {
                E_EmitExpression(emitter, expr->op.call.params[i]);
                if (i != expr->op.call.call_arity - 1)
                    E_Write(emitter, ", ");
            }
            E_Write(emitter, ")");
        } break;
        
        case ExprType_Typename: {
            E_EmitTypeAndName(emitter, &expr->op.typename, (string) {0}, false);
        } break;
        
        case ExprType_Funcname: {
            E_WriteF(emitter, "%.*s", str_expand(expr->op.funcname.name));
        } break;
        
        case ExprType_Lambda: {
            E_WriteF(emitter, "%.*s", str_expand(expr->op.lambda));
        } break;
        
        case ExprType_Cast: {
            E_Write(emitter, "(");
            E_EmitTypeAndName(emitter, &expr->ret_type, (string) {0}, false);
            E_Write(emitter, ")");
            E_EmitExpression(emitter, expr->op.cast);
        } break;
        
        case ExprType_Index: {
            E_EmitExpression(emitter, expr->op.index.operand);
            E_Write(emitter, "[");
            E_EmitExpression(emitter, expr->op.index.index);
            E_Write(emitter, "]");
        } break;
        
        case ExprType_Addr: {
            E_Write(emitter, "&");
            E_EmitExpression(emitter, expr->op.addr);
        } break;
        
        case ExprType_Nullptr: {
            E_Write(emitter, "((void*)0)");
        } break;
        
        case ExprType_Deref: {
            E_Write(emitter, "(*");
            E_EmitExpression(emitter, expr->op.deref);
            E_Write(emitter, ")");
        } break;
        
        case ExprType_Dot: {
            E_EmitExpression(emitter, expr->op.dot.left);
            E_WriteF(emitter, ".%.*s", str_expand(expr->op.dot.right));
        } break;
        
        case ExprType_Arrow: {
            E_EmitExpression(emitter, expr->op.arrow.left);
            E_WriteF(emitter, "->%.*s", str_expand(expr->op.arrow.right));
        } break;
        
        case ExprType_EnumDot: {
            if (expr->op.enum_dot.native) {
                E_WriteF(emitter, "%.*s", str_expand(expr->op.enum_dot.right));
            } else {
                E_WriteF(emitter, "_enum_%.*s_%.*s", str_expand(expr->op.enum_dot.left), str_expand(expr->op.enum_dot.right));
            }
        } break;
        
        case ExprType_Sizeof: {
            E_Write(emitter, "sizeof(");
            E_EmitExpression(emitter, expr->op.sizeof_e);
            E_Write(emitter, ")");
        } break;
        
        case ExprType_Offsetof: {
            E_Write(emitter, "offsetof(");
            E_EmitExpression(emitter, expr->op.offsetof_e.typename);
            E_WriteF(emitter, ", %.*s)", str_expand(expr->op.offsetof_e.member_name));
        } break;
    }
}

static void E_WriteIndent(E_Emitter* emitter, u32 indent) {
    for (int i = 0; i < indent; i++)
        E_Write(emitter, "\t");
}

static void E_EmitStatement(E_Emitter* emitter, P_Stmt* stmt, u32 indent) {
    switch (stmt->type) {
        case StmtType_Expression: {
            E_EmitExpression(emitter, stmt->op.expression);
            E_WriteLine(emitter, ";");
        } break;
        
        case StmtType_Return: {
            E_Write(emitter, "return ");
            E_EmitExpression(emitter, stmt->op.returned);
            E_WriteLine(emitter, ";");
        } break;
        
        case StmtType_Block: {
            E_WriteLine(emitter, "{");
            E_EmitStatementChain(emitter, stmt->op.block, indent + 1);
            E_WriteIndent(emitter, indent);
            E_WriteLine(emitter, "}");
        } break;
        
        case StmtType_VarDecl: {
            P_ValueType type = type_map(emitter, stmt->op.var_decl.type);
            E_EmitTypeAndName(emitter, &type, stmt->op.var_decl.name, false);
            E_WriteLine(emitter, ";");
        } break;
        
        case StmtType_VarDeclAssign: {
            P_ValueType type = type_map(emitter, stmt->op.var_decl_assign.type);
            E_EmitTypeAndName(emitter, &type, stmt->op.var_decl_assign.name, false);
            E_Write(emitter, " = ");
            E_EmitExpression(emitter, stmt->op.var_decl_assign.val);
            E_WriteLine(emitter, ";");
        } break;
        
        case StmtType_FuncDecl: {
            P_ValueType type = type_map(emitter, stmt->op.func_decl.type);
            string varargs;
            string arg_before_varargs;
            E_EmitTypeAndName_Fndecl(emitter, &type, stmt->op.func_decl.name, false, stmt->op.func_decl.param_names, stmt->op.func_decl.param_types, stmt->op.func_decl.arity, stmt->op.func_decl.varargs, &varargs, &arg_before_varargs);
            if (stmt->op.func_decl.block != nullptr) {
                if (stmt->op.func_decl.block->type != StmtType_Block)
                    E_WriteLine(emitter, " {");
                
                if (stmt->op.func_decl.varargs) {
                    E_WriteLineF(emitter, "va_list %.*s;", str_expand(varargs));
                    E_WriteLineF(emitter, "va_start(%.*s, %.*s);", str_expand(varargs), str_expand(arg_before_varargs));
                }
                
                E_EmitStatementChain(emitter, stmt->op.func_decl.block, indent + 1);
                
                // TODO(voxel): Move this to just before any returns
                if (stmt->op.func_decl.varargs)
                    E_WriteLineF(emitter, "va_end(%.*s);", str_expand(varargs));
                
                if (stmt->op.func_decl.block->type != StmtType_Block) {
                    E_WriteIndent(emitter, indent);
                    E_WriteLine(emitter, "}");
                }
            } else {
                E_WriteLine(emitter, "{}");
            }
        } break;
        
        case StmtType_StructDecl: {
            E_WriteLineF(emitter, "typedef struct %.*s %.*s;", str_expand(stmt->op.struct_decl.name), str_expand(stmt->op.struct_decl.name)); E_WriteLineF(emitter, "struct %.*s {", str_expand(stmt->op.struct_decl.name));
            string_list_node* curr_name = stmt->op.struct_decl.member_names.first;
            value_type_list_node* curr_type = stmt->op.struct_decl.member_types.first;
            for (u32 i = 0; i < stmt->op.struct_decl.member_count; i++) {
                P_ValueType c_type = type_map(emitter, curr_type->type);
                
                for (u32 idt = 0; idt < indent + 1; idt++)
                    E_Write(emitter, "\t");
                E_EmitTypeAndName(emitter, &c_type, (string) { .str = curr_name->str, .size = curr_name->size }, false);
                E_WriteLine(emitter, ";");
                
                curr_name = curr_name->next;
                curr_type = curr_type->next;
            }
            E_WriteLine(emitter, "};");
        } break;
        
        case StmtType_UnionDecl: {
            E_WriteLineF(emitter, "typedef union %.*s %.*s;", str_expand(stmt->op.union_decl.name), str_expand(stmt->op.union_decl.name)); E_WriteLineF(emitter, "union %.*s {", str_expand(stmt->op.union_decl.name));
            string_list_node* curr_name = stmt->op.union_decl.member_names.first;
            value_type_list_node* curr_type = stmt->op.union_decl.member_types.first;
            for (u32 i = 0; i < stmt->op.union_decl.member_count; i++) {
                P_ValueType c_type = type_map(emitter, curr_type->type);
                
                for (u32 idt = 0; idt < indent + 1; idt++)
                    E_Write(emitter, "\t");
                E_EmitTypeAndName(emitter, &c_type, (string) { .str = curr_name->str, .size = curr_name->size }, false);
                E_WriteLine(emitter, ";");
                
                curr_name = curr_name->next;
                curr_type = curr_type->next;
            }
            E_WriteLine(emitter, "};");
        } break;
        
        case StmtType_EnumDecl: {
            E_WriteLineF(emitter, "typedef int %.*s;", str_expand(stmt->op.enum_decl.name));
            
            E_WriteLineF(emitter, "enum %.*s {", str_expand(stmt->op.enum_decl.name));
            
            string_list_node* curr_name = stmt->op.enum_decl.member_names.first;
            for (u32 i = 0; i < stmt->op.enum_decl.member_count; i++) {
                for (u32 idt = 0; idt < indent + 1; idt++)
                    E_Write(emitter, "\t");
                E_WriteF(emitter, "_enum_%.*s_%.*s", str_expand(stmt->op.enum_decl.name), str_node_expand(curr_name));
                
                if (stmt->op.enum_decl.member_values[i] != nullptr) {
                    E_Write(emitter, " = ");
                    E_EmitExpression(emitter, stmt->op.enum_decl.member_values[i]);
                }
                
                E_WriteLine(emitter, ",");
                
                curr_name = curr_name->next;
            }
            
            E_WriteLineF(emitter, "};", str_expand(stmt->op.enum_decl.name));
        } break;
        
        case StmtType_FlagEnumDecl: {
            E_WriteLineF(emitter, "typedef int %.*s;", str_expand(stmt->op.flagenum_decl.name));
            
            E_WriteLineF(emitter, "enum %.*s {", str_expand(stmt->op.flagenum_decl.name));
            
            string_list_node* curr_name = stmt->op.flagenum_decl.member_names.first;
            for (u32 i = 0; i < stmt->op.flagenum_decl.member_count; i++) {
                for (u32 idt = 0; idt < indent + 1; idt++)
                    E_Write(emitter, "\t");
                E_WriteF(emitter, "_flagenum_%.*s_%.*s", str_expand(stmt->op.flagenum_decl.name), str_node_expand(curr_name));
                
                if (stmt->op.flagenum_decl.member_values[i] != nullptr) {
                    E_Write(emitter, " = ");
                    E_EmitExpression(emitter, stmt->op.flagenum_decl.member_values[i]);
                }
                
                E_WriteLine(emitter, ",");
                
                curr_name = curr_name->next;
            }
            
            E_WriteLineF(emitter, "};", str_expand(stmt->op.flagenum_decl.name));
        } break;
        
        case StmtType_If: {
            E_Write(emitter, "if (");
            E_EmitExpression(emitter, stmt->op.if_s.condition);
            E_WriteLine(emitter, ")");
            E_EmitStatement(emitter, stmt->op.if_s.then, indent + 1);
        } break;
        
        case StmtType_IfElse: {
            E_Write(emitter, "if (");
            E_EmitExpression(emitter, stmt->op.if_else.condition);
            E_WriteLine(emitter, ")");
            E_EmitStatement(emitter, stmt->op.if_else.then, indent + 1);
            E_WriteLine(emitter, "else");
            E_EmitStatement(emitter, stmt->op.if_else.else_s, indent + 1);
        } break;
        
        case StmtType_For: {
            E_Write(emitter, "for (");
            if (stmt->op.for_s.init != nullptr)
                E_EmitStatement(emitter, stmt->op.for_s.init, 0);
            E_EmitExpression(emitter, stmt->op.for_s.condition);
            E_Write(emitter, ";");
            E_EmitExpression(emitter, stmt->op.for_s.loopexec);
            E_WriteLine(emitter, ")");
            E_EmitStatement(emitter, stmt->op.for_s.then, indent + 1);
        } break;
        
        case StmtType_While: {
            E_Write(emitter, "while (");
            E_EmitExpression(emitter, stmt->op.while_s.condition);
            E_WriteLine(emitter, ")");
            E_EmitStatement(emitter, stmt->op.while_s.then, indent + 1);
        } break;
        
        case StmtType_DoWhile: {
            E_WriteLine(emitter, "do");
            E_EmitStatement(emitter, stmt->op.do_while.then, indent + 1);
            E_Write(emitter, " while (");
            E_EmitExpression(emitter, stmt->op.while_s.condition);
            E_WriteLine(emitter, ");");
        } break;
        
        case StmtType_Switch: {
            E_Write(emitter, "switch (");
            E_EmitExpression(emitter, stmt->op.switch_s.switched);
            E_WriteLine(emitter, ")");
            E_EmitStatement(emitter, stmt->op.switch_s.then, indent + 1);
        } break;
        
        case StmtType_Match: {
            E_Write(emitter, "switch (");
            E_EmitExpression(emitter, stmt->op.match_s.matched);
            E_WriteLine(emitter, ")");
            E_EmitStatement(emitter, stmt->op.match_s.then, indent + 1);
        } break;
        
        case StmtType_Case: {
            E_Write(emitter, "case ");
            E_EmitExpression(emitter, stmt->op.case_s.value);
            E_WriteLine(emitter, ":");
            E_EmitStatementChain(emitter, stmt->op.case_s.then, indent + 1);
        } break;
        
        case StmtType_MatchCase: {
            E_Write(emitter, "case ");
            E_EmitExpression(emitter, stmt->op.mcase_s.value);
            E_WriteLine(emitter, ":");
            if (stmt->op.mcase_s.then->type != StmtType_Block)
                E_WriteLine(emitter, "{");
            E_EmitStatement(emitter, stmt->op.mcase_s.then, indent + 1);
            if (stmt->op.mcase_s.then->type != StmtType_Block)
                E_WriteLine(emitter, "}");
            E_WriteLine(emitter, "break;");
        } break;
        
        case StmtType_Default: {
            E_WriteLine(emitter, "default:");
            E_EmitStatement(emitter, stmt->op.default_s.then, indent + 1);
        } break;
        
        case StmtType_MatchDefault: {
            E_WriteLine(emitter, "default:");
            if (stmt->op.mdefault_s.then->type != StmtType_Block)
                E_WriteLine(emitter, "{");
            E_EmitStatementChain(emitter, stmt->op.mdefault_s.then, indent + 1);
            if (stmt->op.mdefault_s.then->type != StmtType_Block)
                E_WriteLine(emitter, "}");
            E_WriteLine(emitter, "break;");
        } break;
        
        case StmtType_Break: {
            E_WriteLine(emitter, "break;");
        } break;
        
        case StmtType_Continue: {
            E_WriteLine(emitter, "continue;");
        } break;
    }
}

static void E_EmitStatementChain(E_Emitter* emitter, P_Stmt* stmts, u32 indent) {
    for (P_Stmt* stmt = stmts; stmt != nullptr; stmt = stmt->next) {
        E_EmitStatement(emitter, stmt, indent);
    }
}

static void E_EmitPreStatement(E_Emitter* emitter, P_PreStmt* stmt, u32 indent) {
    for (int i = 0; i < indent; i++) E_Write(emitter, "\t");
    
    switch (stmt->type) {
        case PreStmtType_ForwardDecl: {
            P_ValueType type = type_map(emitter, stmt->op.forward_decl.type);
            string _, __;
            E_EmitTypeAndName_Fndecl(emitter, &type, stmt->op.forward_decl.name, false, stmt->op.forward_decl.param_names, stmt->op.forward_decl.param_types, stmt->op.forward_decl.arity, false, &_, &__);
            E_WriteLine(emitter, ";");
        } break;
        
        case PreStmtType_StructForwardDecl: {
            E_WriteLineF(emitter, "typedef struct %.*s %.*s;", str_expand(stmt->op.struct_fd), str_expand(stmt->op.struct_fd));
            E_WriteLineF(emitter, "struct %.*s;", str_expand(stmt->op.struct_fd));
        } break;
        
        case PreStmtType_UnionForwardDecl: {
            E_WriteLineF(emitter, "typedef union %.*s %.*s;", str_expand(stmt->op.union_fd), str_expand(stmt->op.struct_fd));
            E_WriteLineF(emitter, "union %.*s;", str_expand(stmt->op.union_fd));
        } break;
        
        case PreStmtType_CInclude: {
            E_WriteLineF(emitter, "#include \"%.*s\"", str_expand(stmt->op.cinclude));
        } break;
        
        case PreStmtType_CInsert: {
            E_WriteLineF(emitter, "%.*s", str_expand(stmt->op.cinsert));
        } break;
    }
}

static void E_EmitPreStatementChain(E_Emitter* emitter, P_PreStmt* stmts, u32 indent) {
    for (P_PreStmt* stmt = stmts; stmt != nullptr; stmt = stmt->next) {
        E_EmitPreStatement(emitter, stmt, indent);
    }
}

void E_Initialize(E_Emitter* emitter, string source, string filename, string_list tags) {
    P_GlobalInit(tags);
    P_Initialize(&emitter->parser, source, filename, true);
    emitter->output_file = fopen("./generated.c", "w");
}

void E_Emit(E_Emitter* emitter) {
    E_BeginEmitting(emitter);
    P_PreParse(&emitter->parser);
    if (!emitter->parser.had_error) {
        E_EmitPreStatementChain(emitter, emitter->parser.pre_root, 0);
        E_EmitStatementChain(emitter, emitter->parser.root, 0);
    }
    else return;
    
    E_WriteLine(emitter, "");
    
    emitter->parser.root = nullptr;
    emitter->parser.end = nullptr;
    P_Parse(&emitter->parser);
    if (!emitter->parser.had_error) {
        E_EmitStatementChain(emitter, emitter->parser.lambda_functions_start, 0);
        E_EmitStatementChain(emitter, emitter->parser.root, 0);
    }
    E_FinishEmitting(emitter);
}

void E_Free(E_Emitter* emitter) {
    fclose(emitter->output_file);
    P_Free(&emitter->parser);
    P_GlobalFree();
}
