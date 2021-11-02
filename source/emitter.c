#include "emitter.h"
#include <stdarg.h>

// All types will go through this map
static P_ValueType type_map(E_Emitter* emitter, P_ValueType type) {
    type.full_type = str_replace_all(&emitter->parser.arena, type.full_type, str_lit("long"), str_lit("long long"));
    type.full_type = str_replace_all(&emitter->parser.arena, type.full_type, str_lit("string"), str_lit("const char *"));
    return type;
}

static void E_Write(E_Emitter* emitter, const char* text) {
    fprintf(emitter->output_file, text);
}

static void E_WriteLine(E_Emitter* emitter, const char* text) {
    fprintf(emitter->output_file, text);
    fprintf(emitter->output_file, "\n");
}

static void E_WriteF(E_Emitter* emitter, const char* text, ...) {
    va_list args;
    va_start(args, text);
    vfprintf(emitter->output_file, text, args);
    va_end(args);
}

static void E_WriteLineF(E_Emitter* emitter, const char* text, ...) {
    va_list args;
    va_start(args, text);
    vfprintf(emitter->output_file, text, args);
    fprintf(emitter->output_file, "\n");
    va_end(args);
}

// int[6][4]* blah;
// int (*((blah[6])[4]))

static void E_EmitExpression(E_Emitter* emitter, P_Expr* expr);

static void E_EmitTypeMods_Recursive(E_Emitter* emitter, P_ValueType* type, int mod_idx, string name_if_negone) {
    if (mod_idx == -1) {
        E_WriteF(emitter, "%.*s", str_expand(name_if_negone));
        return;
    }
    
    if (mod_idx != 0)
        E_Write(emitter, "(");
    switch (type->mods[mod_idx].type) {
        case ValueTypeModType_Pointer:
        case ValueTypeModType_Reference: {
            E_Write(emitter, "*");
            E_EmitTypeMods_Recursive(emitter, type, mod_idx - 1, name_if_negone);
        } break;
        case ValueTypeModType_Array: {
            E_EmitTypeMods_Recursive(emitter, type, mod_idx - 1, name_if_negone);
            E_Write(emitter, "[");
            E_EmitExpression(emitter, type->mods[mod_idx].op.array_expr);
            E_Write(emitter, "]");
        } break;
    }
    if (mod_idx != 0)
        E_Write(emitter, ")");
}

static void E_EmitTypeAndName(E_Emitter* emitter, P_ValueType* type, string name) {
    E_WriteF(emitter, "%.*s ", str_expand(type->base_type));
    if (type->mod_ct != 0) {
        E_EmitTypeMods_Recursive(emitter, type, type->mod_ct - 1, name);
    } else {
        E_WriteF(emitter, "%.*s", str_expand(name));
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
        
        case ExprType_Typename: {
            E_EmitTypeAndName(emitter, &expr->op.typename, (string) {0});
        } break;
        
        case ExprType_Cast: {
            E_Write(emitter, "(");
            E_EmitTypeAndName(emitter, &expr->ret_type, (string) {0});
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
            E_Write(emitter, "*");
            E_EmitExpression(emitter, expr->op.deref);
        } break;
        
        case ExprType_Dot: {
            E_EmitExpression(emitter, expr->op.dot.left);
            E_WriteF(emitter, ".%.*s", str_expand(expr->op.dot.right));
        } break;
        
        case ExprType_EnumDot: {
            E_WriteF(emitter, "_enum_%.*s_%.*s", str_expand(expr->op.enum_dot.left), str_expand(expr->op.enum_dot.right));
        } break;
    }
}

static void E_EmitStatement(E_Emitter* emitter, P_Stmt* stmt, u32 indent) {
    for (int i = 0; i < indent; i++) E_Write(emitter, "\t");
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
            E_WriteLine(emitter, "}");
        } break;
        
        case StmtType_VarDecl: {
            P_ValueType type = type_map(emitter, stmt->op.var_decl.type);
            E_EmitTypeAndName(emitter, &type, stmt->op.var_decl.name);
            E_WriteLine(emitter, ";");
        } break;
        
        case StmtType_VarDeclAssign: {
            P_ValueType type = type_map(emitter, stmt->op.var_decl_assign.type);
            E_EmitTypeAndName(emitter, &type, stmt->op.var_decl_assign.name);
            E_Write(emitter, " = ");
            E_EmitExpression(emitter, stmt->op.var_decl_assign.val);
            E_WriteLine(emitter, ";");
        } break;
        
        case StmtType_FuncDecl: {
            P_ValueType type = type_map(emitter, stmt->op.func_decl.type);
            
            // FIXME(voxel): Cannot pass Arrays from/to functions
            // NOTE(voxel): This also fixes the function name mangling issue
            E_WriteF(emitter, "%.*s %.*s(", str_expand(type.full_type), str_expand(stmt->op.func_decl.name));
            string_list_node* curr_name = stmt->op.func_decl.param_names.first;
            value_type_list_node* curr_type = stmt->op.func_decl.param_types.first;
            
            string arg_before_varargs;
            string varargs;
            
            if (stmt->op.func_decl.arity != 0) {
                for (u32 i = 0; i < stmt->op.func_decl.arity; i++) {
                    P_ValueType c_type = type_map(emitter, curr_type->type);
                    if (str_eq(c_type.full_type, str_lit("..."))) {
                        E_WriteF(emitter, "%.*s", str_expand(c_type.full_type));
                        varargs = (string) { .str = curr_name->str, .size = curr_name->size };
                        break;
                    }
                    
                    E_EmitTypeAndName(emitter, &c_type, (string) { .str = curr_name->str, .size = curr_name->size });
                    if (i != stmt->op.func_decl.arity - 1)
                        E_Write(emitter, ", ");
                    
                    if (stmt->op.func_decl.varargs) {
                        if (i == stmt->op.func_decl.arity - 2)
                            arg_before_varargs = (string) { .str = curr_name->str, .size = curr_name->size };
                    }
                    
                    curr_name = curr_name->next;
                    curr_type = curr_type->next;
                }
            }
            if (stmt->op.func_decl.arity == 0)
                E_Write(emitter, "void");
            E_WriteLine(emitter, ") {");
            
            if (stmt->op.func_decl.varargs) {
                E_WriteLineF(emitter, "va_list %.*s;", str_expand(varargs));
                E_WriteLineF(emitter, "va_start(%.*s, %.*s);", str_expand(varargs), str_expand(arg_before_varargs));
            }
            
            E_EmitStatementChain(emitter, stmt->op.func_decl.block, indent + 1);
            
            if (stmt->op.func_decl.varargs)
                E_WriteLineF(emitter, "va_end(%.*s);", str_expand(varargs));
            
            E_WriteLine(emitter, "}");
        } break;
        
        case StmtType_StructDecl: {
            E_WriteLineF(emitter, "typedef struct %.*s {", str_expand(stmt->op.struct_decl.name));
            string_list_node* curr_name = stmt->op.struct_decl.member_names.first;
            value_type_list_node* curr_type = stmt->op.struct_decl.member_types.first;
            for (u32 i = 0; i < stmt->op.struct_decl.member_count; i++) {
                P_ValueType c_type = type_map(emitter, curr_type->type);
                
                for (u32 idt = 0; idt < indent + 1; idt++)
                    E_Write(emitter, "\t");
                E_EmitTypeAndName(emitter, &c_type, (string) { .str = curr_name->str, .size = curr_name->size });
                E_Write(emitter, ";");
                
                curr_name = curr_name->next;
                curr_type = curr_type->next;
            }
            E_WriteLineF(emitter, "} %.*s;", str_expand(stmt->op.struct_decl.name));
        } break;
        
        case StmtType_EnumDecl: {
            E_WriteLineF(emitter, "typedef int %.*s;", str_expand(stmt->op.enum_decl.name));
            E_WriteLineF(emitter, "enum %.*s {", str_expand(stmt->op.enum_decl.name));
            string_list_node* curr_name = stmt->op.enum_decl.member_names.first;
            for (u32 i = 0; i < stmt->op.enum_decl.member_count; i++) {
                for (u32 idt = 0; idt < indent + 1; idt++)
                    E_Write(emitter, "\t");
                E_WriteLineF(emitter, "_enum_%.*s_%.*s,", str_expand(stmt->op.enum_decl.name), str_node_expand(curr_name));
                curr_name = curr_name->next;
            }
            E_WriteLineF(emitter, "};", str_expand(stmt->op.enum_decl.name));
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
            E_EmitTypeAndName(emitter, &type, stmt->op.forward_decl.name);
            E_Write(emitter, "(");
            string_list_node* curr_name = stmt->op.forward_decl.param_names.first;
            value_type_list_node* curr_type = stmt->op.forward_decl.param_types.first;
            if (stmt->op.forward_decl.arity != 0) {
                for (u32 i = 0; i < stmt->op.forward_decl.arity; i++) {
                    P_ValueType c_type = type_map(emitter, curr_type->type);
                    if (str_eq(c_type.full_type, str_lit("..."))) {
                        E_WriteF(emitter, "%.*s", c_type.full_type);
                        break;
                    }
                    E_EmitTypeAndName(emitter, &c_type, (string) { .str = curr_name->str, .size = curr_name->size });
                    if (i != stmt->op.forward_decl.arity - 1)
                        E_Write(emitter, ", ");
                    curr_name = curr_name->next;
                    curr_type = curr_type->next;
                }
            }
            if (stmt->op.forward_decl.arity == 0)
                E_Write(emitter, "void");
            E_WriteLine(emitter, ");");
        }
    }
}

static void E_EmitPreStatementChain(E_Emitter* emitter, P_PreStmt* stmts, u32 indent) {
    for (P_PreStmt* stmt = stmts; stmt != nullptr; stmt = stmt->next) {
        E_EmitPreStatement(emitter, stmt, indent);
    }
}

void E_Initialize(E_Emitter* emitter, string source) {
    P_Initialize(&emitter->parser, source);
    emitter->line = 0;
    emitter->output_file = fopen("./generated.c", "w");
}

void E_Emit(E_Emitter* emitter) {
    E_BeginEmitting(emitter);
    P_PreParse(&emitter->parser);
    if (!emitter->parser.had_error)
        E_EmitPreStatementChain(emitter, emitter->parser.pre_root, 0);
    else return;
    
    E_WriteLine(emitter, "");
    
    P_Parse(&emitter->parser);
    if (!emitter->parser.had_error)
        E_EmitStatementChain(emitter, emitter->parser.root, 0);
    E_FinishEmitting(emitter);
}

void E_Free(E_Emitter* emitter) {
    fclose(emitter->output_file);
    P_Free(&emitter->parser);
}
