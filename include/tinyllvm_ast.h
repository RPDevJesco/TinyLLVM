/**
 * ==============================================================================
 * TinyLLVM - Minimal Compiler IR
 * AST (Abstract Syntax Tree) Definitions
 * ==============================================================================
 *
 * This file defines the AST node structures for the CoreTiny language.
 *
 * Language Features:
 * - Types: int, bool
 * - Expressions: literals, variables, binary ops, unary ops, function calls
 * - Statements: var decl, assignment, if/else, while, return, expression stmt
 * - Functions: parameters, return type, body
 * - Program: collection of functions with a main() entry point
 *
 * Copyright (c) 2025 TinyLLVM Project
 * Licensed under the MIT License
 * ==============================================================================
 */

#ifndef TINYLLVM_AST_H
#define TINYLLVM_AST_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==============================================================================
 * Forward Declarations
 * ==============================================================================
 */

typedef struct ASTExpr ASTExpr;
typedef struct ASTStmt ASTStmt;
typedef struct ASTFunc ASTFunc;
typedef struct ASTProgram ASTProgram;

/* ==============================================================================
 * Type System
 * ==============================================================================
 */

typedef enum {
    TYPE_INT,
    TYPE_BOOL,
    TYPE_VOID   /* Only for return type of functions that don't return */
} TypeKind;

typedef struct {
    TypeKind kind;
} Type;

/* ==============================================================================
 * Expression Nodes
 * ==============================================================================
 */

typedef enum {
    /* Literals */
    EXPR_INT_LITERAL,
    EXPR_BOOL_LITERAL,

    /* Variable reference */
    EXPR_VAR,

    /* Binary operations */
    EXPR_ADD,           /* + */
    EXPR_SUB,           /* - */
    EXPR_MUL,           /* * */
    EXPR_DIV,           /* / */
    EXPR_MOD,           /* % */

    /* Comparison operations */
    EXPR_EQ,            /* == */
    EXPR_NE,            /* != */
    EXPR_LT,            /* < */
    EXPR_LE,            /* <= */
    EXPR_GT,            /* > */
    EXPR_GE,            /* >= */

    /* Logical operations */
    EXPR_AND,           /* && */
    EXPR_OR,            /* || */
    EXPR_NOT,           /* ! */

    /* Function call */
    EXPR_CALL
} ExprKind;

/* Integer literal */
typedef struct {
    int value;
} IntLiteral;

/* Boolean literal */
typedef struct {
    bool value;
} BoolLiteral;

/* Variable reference */
typedef struct {
    char *name;
} VarExpr;

/* Binary operation */
typedef struct {
    ASTExpr *left;
    ASTExpr *right;
} BinaryExpr;

/* Unary operation */
typedef struct {
    ASTExpr *operand;
} UnaryExpr;

/* Function call */
typedef struct {
    char *func_name;
    ASTExpr **args;      /* Array of argument expressions */
    size_t arg_count;
} CallExpr;

/* Main expression structure */
struct ASTExpr {
    ExprKind kind;
    Type type;           /* Type of this expression (filled by type checker) */

    union {
        IntLiteral int_lit;
        BoolLiteral bool_lit;
        VarExpr var;
        BinaryExpr binary;
        UnaryExpr unary;
        CallExpr call;
    } data;
};

/* ==============================================================================
 * Statement Nodes
 * ==============================================================================
 */

typedef enum {
    STMT_VAR_DECL,      /* var x = expr; */
    STMT_ASSIGN,        /* x = expr; */
    STMT_IF,            /* if (cond) { ... } [else { ... }] */
    STMT_WHILE,         /* while (cond) { ... } */
    STMT_RETURN,        /* return expr; */
    STMT_EXPR,          /* expr; (expression statement) */
    STMT_BLOCK          /* { stmt1; stmt2; ... } */
} StmtKind;

/* Variable declaration: var name = init_expr; */
typedef struct {
    char *name;
    Type type;
    ASTExpr *init_expr;
} VarDeclStmt;

/* Assignment: name = expr; */
typedef struct {
    char *name;
    ASTExpr *expr;
} AssignStmt;

/* If statement: if (cond) then_block [else else_block] */
typedef struct {
    ASTExpr *condition;
    ASTStmt *then_block;
    ASTStmt *else_block;  /* May be NULL */
} IfStmt;

/* While statement: while (cond) body */
typedef struct {
    ASTExpr *condition;
    ASTStmt *body;
} WhileStmt;

/* Return statement: return expr; */
typedef struct {
    ASTExpr *expr;  /* May be NULL for void return */
} ReturnStmt;

/* Expression statement: expr; */
typedef struct {
    ASTExpr *expr;
} ExprStmt;

/* Block statement: { stmt1; stmt2; ... } */
typedef struct {
    ASTStmt **statements;
    size_t stmt_count;
} BlockStmt;

/* Main statement structure */
struct ASTStmt {
    StmtKind kind;

    union {
        VarDeclStmt var_decl;
        AssignStmt assign;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ReturnStmt return_stmt;
        ExprStmt expr_stmt;
        BlockStmt block;
    } data;
};

/* ==============================================================================
 * Function & Program Nodes
 * ==============================================================================
 */

/* Function parameter */
typedef struct {
    char *name;
    Type type;
} Param;

/* Function definition */
struct ASTFunc {
    char *name;
    Param *params;
    size_t param_count;
    Type return_type;
    ASTStmt *body;  /* Should be a STMT_BLOCK */
};

/* Program (collection of functions) */
struct ASTProgram {
    ASTFunc **functions;
    size_t func_count;
};

/* ==============================================================================
 * AST Construction Functions
 * ==============================================================================
 */

/* Expression constructors */
ASTExpr *ast_expr_int_literal(int value);
ASTExpr *ast_expr_bool_literal(bool value);
ASTExpr *ast_expr_var(const char *name);
ASTExpr *ast_expr_binary(ExprKind kind, ASTExpr *left, ASTExpr *right);
ASTExpr *ast_expr_unary(ExprKind kind, ASTExpr *operand);
ASTExpr *ast_expr_call(const char *func_name, ASTExpr **args, size_t arg_count);

/* Statement constructors */
ASTStmt *ast_stmt_var_decl(const char *name, Type type, ASTExpr *init_expr);
ASTStmt *ast_stmt_assign(const char *name, ASTExpr *expr);
ASTStmt *ast_stmt_if(ASTExpr *condition, ASTStmt *then_block, ASTStmt *else_block);
ASTStmt *ast_stmt_while(ASTExpr *condition, ASTStmt *body);
ASTStmt *ast_stmt_return(ASTExpr *expr);
ASTStmt *ast_stmt_expr(ASTExpr *expr);
ASTStmt *ast_stmt_block(ASTStmt **statements, size_t stmt_count);

/* Function constructor */
ASTFunc *ast_func_create(const char *name, Param *params, size_t param_count,
                         Type return_type, ASTStmt *body);

/* Program constructor */
ASTProgram *ast_program_create(ASTFunc **functions, size_t func_count);

/* ==============================================================================
 * AST Destruction Functions
 * ==============================================================================
 */

void ast_expr_destroy(ASTExpr *expr);
void ast_stmt_destroy(ASTStmt *stmt);
void ast_func_destroy(ASTFunc *func);
void ast_program_destroy(ASTProgram *program);

/* ==============================================================================
 * AST Printing (for debugging)
 * ==============================================================================
 */

void ast_expr_print(const ASTExpr *expr, int indent);
void ast_stmt_print(const ASTStmt *stmt, int indent);
void ast_func_print(const ASTFunc *func, int indent);
void ast_program_print(const ASTProgram *program);

/* ==============================================================================
 * Type Helper Functions
 * ==============================================================================
 */

Type type_int(void);
Type type_bool(void);
Type type_void(void);
const char *type_to_string(Type type);
bool type_equals(Type a, Type b);

#ifdef __cplusplus
}
#endif

#endif /* TINYLLVM_AST_H */