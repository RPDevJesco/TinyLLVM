/**
 * ==============================================================================
 * TinyLLVM Compiler - Type Checker Implementation
 * ==============================================================================
 * 
 * Validates types and annotates the AST with type information.
 * 
 * Type Rules:
 * - int: integer values
 * - bool: true/false
 * - Arithmetic ops (+, -, *, /, %): int × int → int
 * - Comparisons (<, <=, >, >=): int × int → bool
 * - Equality (==, !=): T × T → bool (where both sides have same type)
 * - Logical (&&, ||): bool × bool → bool
 * - Unary (!): bool → bool
 * - Variables must be declared before use
 * - Function calls must match parameter types
 */

#include "include/tinyllvm_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ==============================================================================
 * Symbol Table
 * ==============================================================================
 */

typedef struct Symbol {
    char *name;
    Type type;
    bool is_function;
    size_t param_count;
    Type *param_types;
} Symbol;

typedef struct SymbolTable {
    Symbol *symbols;
    size_t count;
    size_t capacity;
    struct SymbolTable *parent;  /* For nested scopes */
} SymbolTable;

static SymbolTable *symbol_table_create(SymbolTable *parent) {
    SymbolTable *table = malloc(sizeof(SymbolTable));
    if (!table) return NULL;
    
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
    table->parent = parent;
    
    return table;
}

static void symbol_table_destroy(SymbolTable *table) {
    if (!table) return;
    
    for (size_t i = 0; i < table->count; i++) {
        free(table->symbols[i].name);
        free(table->symbols[i].param_types);
    }
    free(table->symbols);
    free(table);
}

static Symbol *symbol_table_lookup(SymbolTable *table, const char *name) {
    /* Search current scope */
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }
    
    /* Search parent scopes */
    if (table->parent) {
        return symbol_table_lookup(table->parent, name);
    }
    
    return NULL;
}

static bool symbol_table_add(SymbolTable *table, const char *name, Type type,
                             bool is_function, size_t param_count, Type *param_types) {
    /* Check for duplicate in current scope */
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return false;  /* Already defined */
        }
    }
    
    /* Grow array if needed */
    if (table->count >= table->capacity) {
        size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
        Symbol *new_symbols = realloc(table->symbols, new_capacity * sizeof(Symbol));
        if (!new_symbols) return false;
        
        table->symbols = new_symbols;
        table->capacity = new_capacity;
    }
    
    /* Add symbol */
    Symbol *sym = &table->symbols[table->count++];
    sym->name = strdup(name);
    sym->type = type;
    sym->is_function = is_function;
    sym->param_count = param_count;
    sym->param_types = param_types;
    
    return true;
}

/* ==============================================================================
 * Type Checker State
 * ==============================================================================
 */

typedef struct {
    SymbolTable *globals;
    SymbolTable *current_scope;
    Type current_function_return_type;
    
    char error_msg[1024];
    bool has_error;
} TypeChecker;

static void type_error(TypeChecker *tc, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(tc->error_msg, sizeof(tc->error_msg), format, args);
    va_end(args);
    tc->has_error = true;
}

/* ==============================================================================
 * Forward Declarations
 * ==============================================================================
 */

static bool check_expression(TypeChecker *tc, ASTExpr *expr);
static bool check_statement(TypeChecker *tc, ASTStmt *stmt);

/* ==============================================================================
 * Type Checking - Expressions
 * ==============================================================================
 */

static bool check_expression(TypeChecker *tc, ASTExpr *expr) {
    if (!expr) return false;
    
    switch (expr->kind) {
        case EXPR_INT_LITERAL:
            expr->type = type_int();
            return true;
            
        case EXPR_BOOL_LITERAL:
            expr->type = type_bool();
            return true;
            
        case EXPR_VAR: {
            Symbol *sym = symbol_table_lookup(tc->current_scope, expr->data.var.name);
            if (!sym) {
                type_error(tc, "Undefined variable '%s'", expr->data.var.name);
                return false;
            }
            if (sym->is_function) {
                type_error(tc, "'%s' is a function, not a variable", expr->data.var.name);
                return false;
            }
            expr->type = sym->type;
            return true;
        }
        
        case EXPR_ADD:
        case EXPR_SUB:
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_MOD: {
            if (!check_expression(tc, expr->data.binary.left)) return false;
            if (!check_expression(tc, expr->data.binary.right)) return false;
            
            if (expr->data.binary.left->type.kind != TYPE_INT) {
                type_error(tc, "Arithmetic operator requires int, got %s",
                          type_to_string(expr->data.binary.left->type));
                return false;
            }
            if (expr->data.binary.right->type.kind != TYPE_INT) {
                type_error(tc, "Arithmetic operator requires int, got %s",
                          type_to_string(expr->data.binary.right->type));
                return false;
            }
            
            expr->type = type_int();
            return true;
        }
        
        case EXPR_LT:
        case EXPR_LE:
        case EXPR_GT:
        case EXPR_GE: {
            if (!check_expression(tc, expr->data.binary.left)) return false;
            if (!check_expression(tc, expr->data.binary.right)) return false;
            
            if (expr->data.binary.left->type.kind != TYPE_INT) {
                type_error(tc, "Comparison requires int, got %s",
                          type_to_string(expr->data.binary.left->type));
                return false;
            }
            if (expr->data.binary.right->type.kind != TYPE_INT) {
                type_error(tc, "Comparison requires int, got %s",
                          type_to_string(expr->data.binary.right->type));
                return false;
            }
            
            expr->type = type_bool();
            return true;
        }
        
        case EXPR_EQ:
        case EXPR_NE: {
            if (!check_expression(tc, expr->data.binary.left)) return false;
            if (!check_expression(tc, expr->data.binary.right)) return false;
            
            if (!type_equals(expr->data.binary.left->type, expr->data.binary.right->type)) {
                type_error(tc, "Equality comparison requires same types, got %s and %s",
                          type_to_string(expr->data.binary.left->type),
                          type_to_string(expr->data.binary.right->type));
                return false;
            }
            
            expr->type = type_bool();
            return true;
        }
        
        case EXPR_AND:
        case EXPR_OR: {
            if (!check_expression(tc, expr->data.binary.left)) return false;
            if (!check_expression(tc, expr->data.binary.right)) return false;
            
            if (expr->data.binary.left->type.kind != TYPE_BOOL) {
                type_error(tc, "Logical operator requires bool, got %s",
                          type_to_string(expr->data.binary.left->type));
                return false;
            }
            if (expr->data.binary.right->type.kind != TYPE_BOOL) {
                type_error(tc, "Logical operator requires bool, got %s",
                          type_to_string(expr->data.binary.right->type));
                return false;
            }
            
            expr->type = type_bool();
            return true;
        }
        
        case EXPR_NOT: {
            if (!check_expression(tc, expr->data.unary.operand)) return false;
            
            if (expr->data.unary.operand->type.kind != TYPE_BOOL) {
                type_error(tc, "Logical NOT requires bool, got %s",
                          type_to_string(expr->data.unary.operand->type));
                return false;
            }
            
            expr->type = type_bool();
            return true;
        }
        
        case EXPR_CALL: {
            Symbol *sym = symbol_table_lookup(tc->current_scope, expr->data.call.func_name);
            if (!sym) {
                type_error(tc, "Undefined function '%s'", expr->data.call.func_name);
                return false;
            }
            if (!sym->is_function) {
                type_error(tc, "'%s' is not a function", expr->data.call.func_name);
                return false;
            }
            
            /* Check argument count */
            if (expr->data.call.arg_count != sym->param_count) {
                type_error(tc, "Function '%s' expects %zu arguments, got %zu",
                          expr->data.call.func_name, sym->param_count,
                          expr->data.call.arg_count);
                return false;
            }
            
            /* Check argument types */
            for (size_t i = 0; i < expr->data.call.arg_count; i++) {
                if (!check_expression(tc, expr->data.call.args[i])) return false;
                
                if (!type_equals(expr->data.call.args[i]->type, sym->param_types[i])) {
                    type_error(tc, "Function '%s' parameter %zu expects %s, got %s",
                              expr->data.call.func_name, i + 1,
                              type_to_string(sym->param_types[i]),
                              type_to_string(expr->data.call.args[i]->type));
                    return false;
                }
            }
            
            expr->type = sym->type;
            return true;
        }
        
        default:
            type_error(tc, "Unknown expression kind");
            return false;
    }
}

/* ==============================================================================
 * Type Checking - Statements
 * ==============================================================================
 */

static bool check_statement(TypeChecker *tc, ASTStmt *stmt) {
    if (!stmt) return false;
    
    switch (stmt->kind) {
        case STMT_VAR_DECL: {
            if (!check_expression(tc, stmt->data.var_decl.init_expr)) return false;
            
            /* Update type from initializer */
            stmt->data.var_decl.type = stmt->data.var_decl.init_expr->type;
            
            /* Add to symbol table */
            if (!symbol_table_add(tc->current_scope, stmt->data.var_decl.name,
                                 stmt->data.var_decl.type, false, 0, NULL)) {
                type_error(tc, "Variable '%s' already declared", stmt->data.var_decl.name);
                return false;
            }
            
            return true;
        }
        
        case STMT_ASSIGN: {
            Symbol *sym = symbol_table_lookup(tc->current_scope, stmt->data.assign.name);
            if (!sym) {
                type_error(tc, "Undefined variable '%s'", stmt->data.assign.name);
                return false;
            }
            if (sym->is_function) {
                type_error(tc, "Cannot assign to function '%s'", stmt->data.assign.name);
                return false;
            }
            
            if (!check_expression(tc, stmt->data.assign.expr)) return false;
            
            if (!type_equals(sym->type, stmt->data.assign.expr->type)) {
                type_error(tc, "Cannot assign %s to variable of type %s",
                          type_to_string(stmt->data.assign.expr->type),
                          type_to_string(sym->type));
                return false;
            }
            
            return true;
        }
        
        case STMT_IF: {
            if (!check_expression(tc, stmt->data.if_stmt.condition)) return false;
            
            if (stmt->data.if_stmt.condition->type.kind != TYPE_BOOL) {
                type_error(tc, "If condition must be bool, got %s",
                          type_to_string(stmt->data.if_stmt.condition->type));
                return false;
            }
            
            if (!check_statement(tc, stmt->data.if_stmt.then_block)) return false;
            
            if (stmt->data.if_stmt.else_block) {
                if (!check_statement(tc, stmt->data.if_stmt.else_block)) return false;
            }
            
            return true;
        }
        
        case STMT_WHILE: {
            if (!check_expression(tc, stmt->data.while_stmt.condition)) return false;
            
            if (stmt->data.while_stmt.condition->type.kind != TYPE_BOOL) {
                type_error(tc, "While condition must be bool, got %s",
                          type_to_string(stmt->data.while_stmt.condition->type));
                return false;
            }
            
            if (!check_statement(tc, stmt->data.while_stmt.body)) return false;
            
            return true;
        }
        
        case STMT_RETURN: {
            if (stmt->data.return_stmt.expr) {
                if (!check_expression(tc, stmt->data.return_stmt.expr)) return false;
                
                if (!type_equals(stmt->data.return_stmt.expr->type,
                               tc->current_function_return_type)) {
                    type_error(tc, "Return type mismatch: expected %s, got %s",
                              type_to_string(tc->current_function_return_type),
                              type_to_string(stmt->data.return_stmt.expr->type));
                    return false;
                }
            } else {
                if (tc->current_function_return_type.kind != TYPE_VOID) {
                    type_error(tc, "Function must return %s",
                              type_to_string(tc->current_function_return_type));
                    return false;
                }
            }
            
            return true;
        }
        
        case STMT_EXPR: {
            return check_expression(tc, stmt->data.expr_stmt.expr);
        }
        
        case STMT_BLOCK: {
            /* Create new scope */
            SymbolTable *prev_scope = tc->current_scope;
            tc->current_scope = symbol_table_create(prev_scope);
            
            bool success = true;
            for (size_t i = 0; i < stmt->data.block.stmt_count; i++) {
                if (!check_statement(tc, stmt->data.block.statements[i])) {
                    success = false;
                    break;
                }
            }
            
            /* Restore scope */
            symbol_table_destroy(tc->current_scope);
            tc->current_scope = prev_scope;
            
            return success;
        }
        
        default:
            type_error(tc, "Unknown statement kind");
            return false;
    }
}

/* ==============================================================================
 * Type Checking - Functions
 * ==============================================================================
 */

static bool check_function(TypeChecker *tc, ASTFunc *func) {
    /* Add function to global scope */
    Type *param_types = malloc(func->param_count * sizeof(Type));
    if (!param_types && func->param_count > 0) {
        type_error(tc, "Out of memory");
        return false;
    }
    
    for (size_t i = 0; i < func->param_count; i++) {
        param_types[i] = func->params[i].type;
    }
    
    if (!symbol_table_add(tc->globals, func->name, func->return_type,
                         true, func->param_count, param_types)) {
        free(param_types);
        type_error(tc, "Function '%s' already declared", func->name);
        return false;
    }
    
    /* Create function scope */
    SymbolTable *func_scope = symbol_table_create(tc->globals);
    if (!func_scope) {
        type_error(tc, "Out of memory");
        return false;
    }
    
    /* Add parameters to function scope */
    for (size_t i = 0; i < func->param_count; i++) {
        if (!symbol_table_add(func_scope, func->params[i].name,
                             func->params[i].type, false, 0, NULL)) {
            symbol_table_destroy(func_scope);
            type_error(tc, "Duplicate parameter '%s'", func->params[i].name);
            return false;
        }
    }
    
    /* Check function body */
    tc->current_scope = func_scope;
    tc->current_function_return_type = func->return_type;
    
    bool success = check_statement(tc, func->body);
    
    symbol_table_destroy(func_scope);
    tc->current_scope = tc->globals;
    
    return success;
}

/* ==============================================================================
 * Public Type Checker API
 * ==============================================================================
 */

bool type_check_program(ASTProgram *program, char *error_msg, size_t error_msg_size) {
    if (!program) {
        if (error_msg) {
            snprintf(error_msg, error_msg_size, "No program to type check");
        }
        return false;
    }
    
    TypeChecker tc = {
        .globals = symbol_table_create(NULL),
        .current_scope = NULL,
        .has_error = false
    };
    tc.error_msg[0] = '\0';
    
    if (!tc.globals) {
        if (error_msg) {
            snprintf(error_msg, error_msg_size, "Out of memory");
        }
        return false;
    }
    
    tc.current_scope = tc.globals;
    
    /* Add built-in print function */
    Type *print_params = malloc(sizeof(Type));
    if (print_params) {
        print_params[0] = type_int();
        symbol_table_add(tc.globals, "print", type_void(), true, 1, print_params);
    }
    
    /* First pass: register all function signatures */
    for (size_t i = 0; i < program->func_count; i++) {
        /* Just add to symbol table, don't check body yet */
        ASTFunc *func = program->functions[i];
        
        Type *param_types = malloc(func->param_count * sizeof(Type));
        if (!param_types && func->param_count > 0) {
            symbol_table_destroy(tc.globals);
            if (error_msg) {
                snprintf(error_msg, error_msg_size, "Out of memory");
            }
            return false;
        }
        
        for (size_t j = 0; j < func->param_count; j++) {
            param_types[j] = func->params[j].type;
        }
        
        if (!symbol_table_add(tc.globals, func->name, func->return_type,
                             true, func->param_count, param_types)) {
            free(param_types);
            symbol_table_destroy(tc.globals);
            if (error_msg) {
                snprintf(error_msg, error_msg_size,
                        "Duplicate function '%s'", func->name);
            }
            return false;
        }
    }
    
    /* Second pass: check function bodies */
    for (size_t i = 0; i < program->func_count; i++) {
        /* Create function scope */
        SymbolTable *func_scope = symbol_table_create(tc.globals);
        
        ASTFunc *func = program->functions[i];
        
        /* Add parameters */
        for (size_t j = 0; j < func->param_count; j++) {
            symbol_table_add(func_scope, func->params[j].name,
                           func->params[j].type, false, 0, NULL);
        }
        
        /* Check body */
        tc.current_scope = func_scope;
        tc.current_function_return_type = func->return_type;
        
        if (!check_statement(&tc, func->body)) {
            symbol_table_destroy(func_scope);
            symbol_table_destroy(tc.globals);
            
            if (error_msg) {
                snprintf(error_msg, error_msg_size, "%s", tc.error_msg);
            }
            return false;
        }
        
        symbol_table_destroy(func_scope);
    }
    
    symbol_table_destroy(tc.globals);
    return true;
}

/* ==============================================================================
 * Type Checker Event (EventChains Integration)
 * ==============================================================================
 */

EventResult compiler_type_checker_event(EventContext *context, void *user_data) {
    (void)user_data;
    
    EventResult result;
    
    /* Get AST from context */
    ASTProgram *program;
    EventChainErrorCode err = event_context_get(context, "ast", (void**)&program);
    
    if (err != EC_SUCCESS || !program) {
        event_result_failure(&result, "No AST provided to type checker",
                           EC_ERROR_INVALID_PARAMETER, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Type check the program */
    char error_msg[1024];
    if (!type_check_program(program, error_msg, sizeof(error_msg))) {
        char full_msg[1100];
        snprintf(full_msg, sizeof(full_msg), "Type checking failed: %s", error_msg);
        event_result_failure(&result, full_msg,
                           EC_ERROR_INVALID_PARAMETER, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* AST is modified in-place with type information */
    /* No need to update context */
    
    /* Success */
    event_result_success(&result);
    return result;
}
