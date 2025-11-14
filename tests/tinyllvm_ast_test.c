/**
 * ==============================================================================
 * TinyLLVM - AST Test Program
 * ==============================================================================
 * 
 * This program demonstrates how to manually construct an AST and print it.
 * It creates a simple program:
 * 
 * func factorial(n: int) : int {
 *     var result = 1;
 *     while (n > 1) {
 *         result = result * n;
 *         n = n - 1;
 *     }
 *     return result;
 * }
 * 
 * func main() : int {
 *     var x = 5;
 *     var fact = factorial(x);
 *     print(fact);
 *     return 0;
 * }
 */

#include "include/tinyllvm_ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper to create a parameter */
static Param *create_param(const char *name, Type type) {
    Param *param = malloc(sizeof(Param));
    if (!param) return NULL;
    
    param->name = malloc(strlen(name) + 1);
    if (!param->name) {
        free(param);
        return NULL;
    }
    strcpy(param->name, name);
    param->type = type;
    
    return param;
}

/* Build the factorial function */
static ASTFunc *build_factorial_func(void) {
    /* Parameter: n:int */
    Param *params = create_param("n", type_int());
    
    /* var result = 1; */
    ASTStmt *var_result = ast_stmt_var_decl("result", type_int(), 
                                            ast_expr_int_literal(1));
    
    /* n > 1 */
    ASTExpr *while_cond = ast_expr_binary(EXPR_GT,
                                          ast_expr_var("n"),
                                          ast_expr_int_literal(1));
    
    /* result = result * n; */
    ASTStmt *assign_result = ast_stmt_assign("result",
                                             ast_expr_binary(EXPR_MUL,
                                                           ast_expr_var("result"),
                                                           ast_expr_var("n")));
    
    /* n = n - 1; */
    ASTStmt *assign_n = ast_stmt_assign("n",
                                       ast_expr_binary(EXPR_SUB,
                                                     ast_expr_var("n"),
                                                     ast_expr_int_literal(1)));
    
    /* while body block */
    ASTStmt **while_body_stmts = malloc(2 * sizeof(ASTStmt*));
    while_body_stmts[0] = assign_result;
    while_body_stmts[1] = assign_n;
    ASTStmt *while_body = ast_stmt_block(while_body_stmts, 2);
    
    /* while statement */
    ASTStmt *while_stmt = ast_stmt_while(while_cond, while_body);
    
    /* return result; */
    ASTStmt *return_stmt = ast_stmt_return(ast_expr_var("result"));
    
    /* Function body block */
    ASTStmt **func_body_stmts = malloc(3 * sizeof(ASTStmt*));
    func_body_stmts[0] = var_result;
    func_body_stmts[1] = while_stmt;
    func_body_stmts[2] = return_stmt;
    ASTStmt *func_body = ast_stmt_block(func_body_stmts, 3);
    
    /* Create function */
    return ast_func_create("factorial", params, 1, type_int(), func_body);
}

/* Build the main function */
static ASTFunc *build_main_func(void) {
    /* var x = 5; */
    ASTStmt *var_x = ast_stmt_var_decl("x", type_int(),
                                       ast_expr_int_literal(5));
    
    /* factorial(x) */
    ASTExpr **call_args = malloc(sizeof(ASTExpr*));
    call_args[0] = ast_expr_var("x");
    ASTExpr *factorial_call = ast_expr_call("factorial", call_args, 1);
    
    /* var fact = factorial(x); */
    ASTStmt *var_fact = ast_stmt_var_decl("fact", type_int(), factorial_call);
    
    /* print(fact) */
    ASTExpr **print_args = malloc(sizeof(ASTExpr*));
    print_args[0] = ast_expr_var("fact");
    ASTExpr *print_call = ast_expr_call("print", print_args, 1);
    ASTStmt *print_stmt = ast_stmt_expr(print_call);
    
    /* return 0; */
    ASTStmt *return_stmt = ast_stmt_return(ast_expr_int_literal(0));
    
    /* Function body block */
    ASTStmt **func_body_stmts = malloc(4 * sizeof(ASTStmt*));
    func_body_stmts[0] = var_x;
    func_body_stmts[1] = var_fact;
    func_body_stmts[2] = print_stmt;
    func_body_stmts[3] = return_stmt;
    ASTStmt *func_body = ast_stmt_block(func_body_stmts, 4);
    
    /* Create function (no parameters) */
    return ast_func_create("main", NULL, 0, type_int(), func_body);
}

int main(void) {
    printf("=== TinyLLVM AST Test ===\n\n");
    
    /* Build both functions */
    ASTFunc *factorial_func = build_factorial_func();
    ASTFunc *main_func = build_main_func();
    
    if (!factorial_func || !main_func) {
        fprintf(stderr, "Error: Failed to create functions\n");
        return 1;
    }
    
    /* Create program */
    ASTFunc **functions = malloc(2 * sizeof(ASTFunc*));
    functions[0] = factorial_func;
    functions[1] = main_func;
    
    ASTProgram *program = ast_program_create(functions, 2);
    
    if (!program) {
        fprintf(stderr, "Error: Failed to create program\n");
        return 1;
    }
    
    /* Print the AST */
    printf("Generated AST:\n");
    printf("==============\n\n");
    ast_program_print(program);
    
    /* Clean up */
    ast_program_destroy(program);
    
    printf("\n=== AST Test Complete ===\n");
    
    return 0;
}
