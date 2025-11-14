/**
 * ==============================================================================
 * TinyLLVM - AST Implementation
 * ==============================================================================
 */

#include "include/tinyllvm_ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ==============================================================================
 * Memory Management Helpers
 * ==============================================================================
 */

static char *str_duplicate(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, str, len + 1);
    }
    return dup;
}

/* ==============================================================================
 * Type Helper Functions
 * ==============================================================================
 */

Type type_int(void) {
    Type t = { .kind = TYPE_INT };
    return t;
}

Type type_bool(void) {
    Type t = { .kind = TYPE_BOOL };
    return t;
}

Type type_void(void) {
    Type t = { .kind = TYPE_VOID };
    return t;
}

const char *type_to_string(Type type) {
    switch (type.kind) {
        case TYPE_INT:  return "int";
        case TYPE_BOOL: return "bool";
        case TYPE_VOID: return "void";
        default:        return "unknown";
    }
}

bool type_equals(Type a, Type b) {
    return a.kind == b.kind;
}

/* ==============================================================================
 * Expression Constructors
 * ==============================================================================
 */

ASTExpr *ast_expr_int_literal(int value) {
    ASTExpr *expr = malloc(sizeof(ASTExpr));
    if (!expr) return NULL;

    expr->kind = EXPR_INT_LITERAL;
    expr->type = type_int();
    expr->data.int_lit.value = value;

    return expr;
}

ASTExpr *ast_expr_bool_literal(bool value) {
    ASTExpr *expr = malloc(sizeof(ASTExpr));
    if (!expr) return NULL;

    expr->kind = EXPR_BOOL_LITERAL;
    expr->type = type_bool();
    expr->data.bool_lit.value = value;

    return expr;
}

ASTExpr *ast_expr_var(const char *name) {
    if (!name) return NULL;

    ASTExpr *expr = malloc(sizeof(ASTExpr));
    if (!expr) return NULL;

    expr->kind = EXPR_VAR;
    expr->type = type_int();  /* Will be fixed by type checker */
    expr->data.var.name = str_duplicate(name);

    if (!expr->data.var.name) {
        free(expr);
        return NULL;
    }

    return expr;
}

ASTExpr *ast_expr_binary(ExprKind kind, ASTExpr *left, ASTExpr *right) {
    if (!left || !right) return NULL;

    ASTExpr *expr = malloc(sizeof(ASTExpr));
    if (!expr) return NULL;

    expr->kind = kind;
    expr->data.binary.left = left;
    expr->data.binary.right = right;

    /* Set type based on operation */
    switch (kind) {
        case EXPR_EQ:
        case EXPR_NE:
        case EXPR_LT:
        case EXPR_LE:
        case EXPR_GT:
        case EXPR_GE:
        case EXPR_AND:
        case EXPR_OR:
            expr->type = type_bool();
            break;
        default:
            expr->type = type_int();
            break;
    }

    return expr;
}

ASTExpr *ast_expr_unary(ExprKind kind, ASTExpr *operand) {
    if (!operand) return NULL;

    ASTExpr *expr = malloc(sizeof(ASTExpr));
    if (!expr) return NULL;

    expr->kind = kind;
    expr->data.unary.operand = operand;
    expr->type = type_bool();  /* Only ! operator for now */

    return expr;
}

ASTExpr *ast_expr_call(const char *func_name, ASTExpr **args, size_t arg_count) {
    if (!func_name) return NULL;

    ASTExpr *expr = malloc(sizeof(ASTExpr));
    if (!expr) return NULL;

    expr->kind = EXPR_CALL;
    expr->type = type_int();  /* Will be fixed by type checker */
    expr->data.call.func_name = str_duplicate(func_name);
    expr->data.call.args = args;
    expr->data.call.arg_count = arg_count;

    if (!expr->data.call.func_name) {
        free(expr);
        return NULL;
    }

    return expr;
}

/* ==============================================================================
 * Statement Constructors
 * ==============================================================================
 */

ASTStmt *ast_stmt_var_decl(const char *name, Type type, ASTExpr *init_expr) {
    if (!name || !init_expr) return NULL;

    ASTStmt *stmt = malloc(sizeof(ASTStmt));
    if (!stmt) return NULL;

    stmt->kind = STMT_VAR_DECL;
    stmt->data.var_decl.name = str_duplicate(name);
    stmt->data.var_decl.type = type;
    stmt->data.var_decl.init_expr = init_expr;

    if (!stmt->data.var_decl.name) {
        free(stmt);
        return NULL;
    }

    return stmt;
}

ASTStmt *ast_stmt_assign(const char *name, ASTExpr *expr) {
    if (!name || !expr) return NULL;

    ASTStmt *stmt = malloc(sizeof(ASTStmt));
    if (!stmt) return NULL;

    stmt->kind = STMT_ASSIGN;
    stmt->data.assign.name = str_duplicate(name);
    stmt->data.assign.expr = expr;

    if (!stmt->data.assign.name) {
        free(stmt);
        return NULL;
    }

    return stmt;
}

ASTStmt *ast_stmt_if(ASTExpr *condition, ASTStmt *then_block, ASTStmt *else_block) {
    if (!condition || !then_block) return NULL;

    ASTStmt *stmt = malloc(sizeof(ASTStmt));
    if (!stmt) return NULL;

    stmt->kind = STMT_IF;
    stmt->data.if_stmt.condition = condition;
    stmt->data.if_stmt.then_block = then_block;
    stmt->data.if_stmt.else_block = else_block;

    return stmt;
}

ASTStmt *ast_stmt_while(ASTExpr *condition, ASTStmt *body) {
    if (!condition || !body) return NULL;

    ASTStmt *stmt = malloc(sizeof(ASTStmt));
    if (!stmt) return NULL;

    stmt->kind = STMT_WHILE;
    stmt->data.while_stmt.condition = condition;
    stmt->data.while_stmt.body = body;

    return stmt;
}

ASTStmt *ast_stmt_return(ASTExpr *expr) {
    ASTStmt *stmt = malloc(sizeof(ASTStmt));
    if (!stmt) return NULL;

    stmt->kind = STMT_RETURN;
    stmt->data.return_stmt.expr = expr;

    return stmt;
}

ASTStmt *ast_stmt_expr(ASTExpr *expr) {
    if (!expr) return NULL;

    ASTStmt *stmt = malloc(sizeof(ASTStmt));
    if (!stmt) return NULL;

    stmt->kind = STMT_EXPR;
    stmt->data.expr_stmt.expr = expr;

    return stmt;
}

ASTStmt *ast_stmt_block(ASTStmt **statements, size_t stmt_count) {
    ASTStmt *stmt = malloc(sizeof(ASTStmt));
    if (!stmt) return NULL;

    stmt->kind = STMT_BLOCK;
    stmt->data.block.statements = statements;
    stmt->data.block.stmt_count = stmt_count;

    return stmt;
}

/* ==============================================================================
 * Function & Program Constructors
 * ==============================================================================
 */

ASTFunc *ast_func_create(const char *name, Param *params, size_t param_count,
                         Type return_type, ASTStmt *body) {
    if (!name || !body) return NULL;

    ASTFunc *func = malloc(sizeof(ASTFunc));
    if (!func) return NULL;

    func->name = str_duplicate(name);
    func->params = params;
    func->param_count = param_count;
    func->return_type = return_type;
    func->body = body;

    if (!func->name) {
        free(func);
        return NULL;
    }

    return func;
}

ASTProgram *ast_program_create(ASTFunc **functions, size_t func_count) {
    ASTProgram *program = malloc(sizeof(ASTProgram));
    if (!program) return NULL;

    program->functions = functions;
    program->func_count = func_count;

    return program;
}

/* ==============================================================================
 * AST Destruction Functions
 * ==============================================================================
 */

void ast_expr_destroy(ASTExpr *expr) {
    if (!expr) return;

    switch (expr->kind) {
        case EXPR_VAR:
            free(expr->data.var.name);
            break;

        case EXPR_ADD:
        case EXPR_SUB:
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_MOD:
        case EXPR_EQ:
        case EXPR_NE:
        case EXPR_LT:
        case EXPR_LE:
        case EXPR_GT:
        case EXPR_GE:
        case EXPR_AND:
        case EXPR_OR:
            ast_expr_destroy(expr->data.binary.left);
            ast_expr_destroy(expr->data.binary.right);
            break;

        case EXPR_NOT:
            ast_expr_destroy(expr->data.unary.operand);
            break;

        case EXPR_CALL:
            free(expr->data.call.func_name);
            for (size_t i = 0; i < expr->data.call.arg_count; i++) {
                ast_expr_destroy(expr->data.call.args[i]);
            }
            free(expr->data.call.args);
            break;

        default:
            /* Literals have no dynamic memory */
            break;
    }

    free(expr);
}

void ast_stmt_destroy(ASTStmt *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case STMT_VAR_DECL:
            free(stmt->data.var_decl.name);
            ast_expr_destroy(stmt->data.var_decl.init_expr);
            break;

        case STMT_ASSIGN:
            free(stmt->data.assign.name);
            ast_expr_destroy(stmt->data.assign.expr);
            break;

        case STMT_IF:
            ast_expr_destroy(stmt->data.if_stmt.condition);
            ast_stmt_destroy(stmt->data.if_stmt.then_block);
            ast_stmt_destroy(stmt->data.if_stmt.else_block);
            break;

        case STMT_WHILE:
            ast_expr_destroy(stmt->data.while_stmt.condition);
            ast_stmt_destroy(stmt->data.while_stmt.body);
            break;

        case STMT_RETURN:
            ast_expr_destroy(stmt->data.return_stmt.expr);
            break;

        case STMT_EXPR:
            ast_expr_destroy(stmt->data.expr_stmt.expr);
            break;

        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->data.block.stmt_count; i++) {
                ast_stmt_destroy(stmt->data.block.statements[i]);
            }
            free(stmt->data.block.statements);
            break;
    }

    free(stmt);
}

void ast_func_destroy(ASTFunc *func) {
    if (!func) return;

    free(func->name);

    if (func->params) {
        for (size_t i = 0; i < func->param_count; i++) {
            free(func->params[i].name);
        }
        free(func->params);
    }

    ast_stmt_destroy(func->body);
    free(func);
}

void ast_program_destroy(ASTProgram *program) {
    if (!program) return;

    for (size_t i = 0; i < program->func_count; i++) {
        ast_func_destroy(program->functions[i]);
    }
    free(program->functions);
    free(program);
}

/* ==============================================================================
 * AST Printing Functions (for debugging)
 * ==============================================================================
 */

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static const char *expr_kind_to_string(ExprKind kind) {
    switch (kind) {
        case EXPR_INT_LITERAL:  return "INT_LIT";
        case EXPR_BOOL_LITERAL: return "BOOL_LIT";
        case EXPR_VAR:          return "VAR";
        case EXPR_ADD:          return "+";
        case EXPR_SUB:          return "-";
        case EXPR_MUL:          return "*";
        case EXPR_DIV:          return "/";
        case EXPR_MOD:          return "%";
        case EXPR_EQ:           return "==";
        case EXPR_NE:           return "!=";
        case EXPR_LT:           return "<";
        case EXPR_LE:           return "<=";
        case EXPR_GT:           return ">";
        case EXPR_GE:           return ">=";
        case EXPR_AND:          return "&&";
        case EXPR_OR:           return "||";
        case EXPR_NOT:          return "!";
        case EXPR_CALL:         return "CALL";
        default:                return "UNKNOWN";
    }
}

void ast_expr_print(const ASTExpr *expr, int indent) {
    if (!expr) return;

    print_indent(indent);

    switch (expr->kind) {
        case EXPR_INT_LITERAL:
            printf("INT(%d)\n", expr->data.int_lit.value);
            break;

        case EXPR_BOOL_LITERAL:
            printf("BOOL(%s)\n", expr->data.bool_lit.value ? "true" : "false");
            break;

        case EXPR_VAR:
            printf("VAR(%s)\n", expr->data.var.name);
            break;

        case EXPR_ADD:
        case EXPR_SUB:
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_MOD:
        case EXPR_EQ:
        case EXPR_NE:
        case EXPR_LT:
        case EXPR_LE:
        case EXPR_GT:
        case EXPR_GE:
        case EXPR_AND:
        case EXPR_OR:
            printf("%s\n", expr_kind_to_string(expr->kind));
            ast_expr_print(expr->data.binary.left, indent + 1);
            ast_expr_print(expr->data.binary.right, indent + 1);
            break;

        case EXPR_NOT:
            printf("!\n");
            ast_expr_print(expr->data.unary.operand, indent + 1);
            break;

        case EXPR_CALL:
            printf("CALL %s\n", expr->data.call.func_name);
            for (size_t i = 0; i < expr->data.call.arg_count; i++) {
                ast_expr_print(expr->data.call.args[i], indent + 1);
            }
            break;
    }
}

void ast_stmt_print(const ASTStmt *stmt, int indent) {
    if (!stmt) return;

    print_indent(indent);

    switch (stmt->kind) {
        case STMT_VAR_DECL:
            printf("VAR %s : %s =\n",
                   stmt->data.var_decl.name,
                   type_to_string(stmt->data.var_decl.type));
            ast_expr_print(stmt->data.var_decl.init_expr, indent + 1);
            break;

        case STMT_ASSIGN:
            printf("ASSIGN %s =\n", stmt->data.assign.name);
            ast_expr_print(stmt->data.assign.expr, indent + 1);
            break;

        case STMT_IF:
            printf("IF\n");
            print_indent(indent);
            printf("  condition:\n");
            ast_expr_print(stmt->data.if_stmt.condition, indent + 2);
            print_indent(indent);
            printf("  then:\n");
            ast_stmt_print(stmt->data.if_stmt.then_block, indent + 2);
            if (stmt->data.if_stmt.else_block) {
                print_indent(indent);
                printf("  else:\n");
                ast_stmt_print(stmt->data.if_stmt.else_block, indent + 2);
            }
            break;

        case STMT_WHILE:
            printf("WHILE\n");
            print_indent(indent);
            printf("  condition:\n");
            ast_expr_print(stmt->data.while_stmt.condition, indent + 2);
            print_indent(indent);
            printf("  body:\n");
            ast_stmt_print(stmt->data.while_stmt.body, indent + 2);
            break;

        case STMT_RETURN:
            printf("RETURN\n");
            if (stmt->data.return_stmt.expr) {
                ast_expr_print(stmt->data.return_stmt.expr, indent + 1);
            }
            break;

        case STMT_EXPR:
            printf("EXPR_STMT\n");
            ast_expr_print(stmt->data.expr_stmt.expr, indent + 1);
            break;

        case STMT_BLOCK:
            printf("BLOCK\n");
            for (size_t i = 0; i < stmt->data.block.stmt_count; i++) {
                ast_stmt_print(stmt->data.block.statements[i], indent + 1);
            }
            break;
    }
}

void ast_func_print(const ASTFunc *func, int indent) {
    if (!func) return;

    print_indent(indent);
    printf("FUNC %s(", func->name);

    for (size_t i = 0; i < func->param_count; i++) {
        printf("%s:%s", func->params[i].name, type_to_string(func->params[i].type));
        if (i < func->param_count - 1) printf(", ");
    }

    printf(") : %s\n", type_to_string(func->return_type));
    ast_stmt_print(func->body, indent + 1);
}

void ast_program_print(const ASTProgram *program) {
    if (!program) return;

    printf("PROGRAM\n");
    for (size_t i = 0; i < program->func_count; i++) {
        ast_func_print(program->functions[i], 1);
        printf("\n");
    }
}