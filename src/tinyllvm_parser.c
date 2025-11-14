/**
 * ==============================================================================
 * TinyLLVM Compiler - Parser Implementation
 * ==============================================================================
 * 
 * Recursive descent parser that converts tokens into an AST.
 * 
 * Grammar:
 *   Program  ::= { Function }
 *   Function ::= "func" Ident "(" [Params] ")" ":" Type Block
 *   Params   ::= Param { "," Param }
 *   Param    ::= Ident ":" Type
 *   Block    ::= "{" { Stmt } "}"
 *   Stmt     ::= VarDecl | Assign | If | While | Return | ExprStmt
 *   VarDecl  ::= "var" Ident "=" Expr ";"
 *   Assign   ::= Ident "=" Expr ";"
 *   If       ::= "if" "(" Expr ")" Block ["else" Block]
 *   While    ::= "while" "(" Expr ")" Block
 *   Return   ::= "return" Expr ";"
 *   ExprStmt ::= Expr ";"
 *   Expr     ::= LogicalOr
 *   LogicalOr  ::= LogicalAnd { "||" LogicalAnd }
 *   LogicalAnd ::= Equality { "&&" Equality }
 *   Equality   ::= Comparison { ("==" | "!=") Comparison }
 *   Comparison ::= Term { ("<" | "<=" | ">" | ">=") Term }
 *   Term       ::= Factor { ("+" | "-") Factor }
 *   Factor     ::= Unary { ("*" | "/" | "%") Unary }
 *   Unary      ::= "!" Unary | Primary
 *   Primary    ::= IntLit | BoolLit | Ident | Call | "(" Expr ")"
 *   Call       ::= Ident "(" [Args] ")"
 *   Args       ::= Expr { "," Expr }
 */

#include "include/tinyllvm_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ==============================================================================
 * Parser State
 * ==============================================================================
 */

typedef struct {
    Token **tokens;
    size_t count;
    size_t current;
    
    /* Error handling */
    char error_msg[1024];
    bool has_error;
} Parser;

/* ==============================================================================
 * Parser Utilities
 * ==============================================================================
 */

static Token *parser_current(Parser *p) {
    if (p->current >= p->count) return NULL;
    return p->tokens[p->current];
}

static Token *parser_previous(Parser *p) {
    if (p->current == 0) return NULL;
    return p->tokens[p->current - 1];
}

static bool parser_is_at_end(Parser *p) {
    Token *tok = parser_current(p);
    return tok == NULL || tok->kind == TOKEN_EOF;
}

static bool parser_check(Parser *p, TokenKind kind) {
    if (parser_is_at_end(p)) return false;
    return parser_current(p)->kind == kind;
}

static Token *parser_advance(Parser *p) {
    if (!parser_is_at_end(p)) {
        p->current++;
    }
    return parser_previous(p);
}

static bool parser_match(Parser *p, TokenKind kind) {
    if (parser_check(p, kind)) {
        parser_advance(p);
        return true;
    }
    return false;
}

static Token *parser_expect(Parser *p, TokenKind kind, const char *message) {
    if (parser_check(p, kind)) {
        return parser_advance(p);
    }
    
    Token *tok = parser_current(p);
    if (tok) {
        snprintf(p->error_msg, sizeof(p->error_msg),
                "%s at line %zu, column %zu. Got '%s'",
                message, tok->line, tok->column,
                token_kind_to_string(tok->kind));
    } else {
        snprintf(p->error_msg, sizeof(p->error_msg),
                "%s at end of file", message);
    }
    
    p->has_error = true;
    return NULL;
}

/* ==============================================================================
 * Forward Declarations
 * ==============================================================================
 */

static ASTExpr *parse_expression(Parser *p);
static ASTStmt *parse_statement(Parser *p);
static ASTStmt *parse_block(Parser *p);

/* ==============================================================================
 * Expression Parsing
 * ==============================================================================
 */

static ASTExpr *parse_primary(Parser *p) {
    /* Integer literal */
    if (parser_match(p, TOKEN_INT_LITERAL)) {
        Token *tok = parser_previous(p);
        return ast_expr_int_literal(tok->value);
    }
    
    /* Boolean literals */
    if (parser_match(p, TOKEN_TRUE)) {
        return ast_expr_bool_literal(true);
    }
    if (parser_match(p, TOKEN_FALSE)) {
        return ast_expr_bool_literal(false);
    }
    
    /* Identifier or function call */
    if (parser_match(p, TOKEN_IDENTIFIER)) {
        Token *name_tok = parser_previous(p);
        
        /* Function call */
        if (parser_match(p, TOKEN_LPAREN)) {
            /* Parse arguments */
            ASTExpr **args = NULL;
            size_t arg_count = 0;
            size_t arg_capacity = 0;
            
            if (!parser_check(p, TOKEN_RPAREN)) {
                do {
                    ASTExpr *arg = parse_expression(p);
                    if (!arg) return NULL;
                    
                    /* Grow array if needed */
                    if (arg_count >= arg_capacity) {
                        size_t new_capacity = arg_capacity == 0 ? 4 : arg_capacity * 2;
                        ASTExpr **new_args = realloc(args, new_capacity * sizeof(ASTExpr*));
                        if (!new_args) {
                            snprintf(p->error_msg, sizeof(p->error_msg),
                                    "Out of memory parsing function arguments");
                            p->has_error = true;
                            free(args);
                            return NULL;
                        }
                        args = new_args;
                        arg_capacity = new_capacity;
                    }
                    
                    args[arg_count++] = arg;
                } while (parser_match(p, TOKEN_COMMA));
            }
            
            if (!parser_expect(p, TOKEN_RPAREN, "Expected ')' after arguments")) {
                for (size_t i = 0; i < arg_count; i++) {
                    ast_expr_destroy(args[i]);
                }
                free(args);
                return NULL;
            }
            
            return ast_expr_call(name_tok->lexeme, args, arg_count);
        }
        
        /* Just an identifier */
        return ast_expr_var(name_tok->lexeme);
    }
    
    /* Parenthesized expression */
    if (parser_match(p, TOKEN_LPAREN)) {
        ASTExpr *expr = parse_expression(p);
        if (!expr) return NULL;
        
        if (!parser_expect(p, TOKEN_RPAREN, "Expected ')' after expression")) {
            ast_expr_destroy(expr);
            return NULL;
        }
        
        return expr;
    }
    
    Token *tok = parser_current(p);
    if (tok) {
        snprintf(p->error_msg, sizeof(p->error_msg),
                "Expected expression at line %zu, column %zu. Got '%s'",
                tok->line, tok->column, token_kind_to_string(tok->kind));
    } else {
        snprintf(p->error_msg, sizeof(p->error_msg),
                "Expected expression but reached end of file");
    }
    p->has_error = true;
    return NULL;
}

static ASTExpr *parse_unary(Parser *p) {
    if (parser_match(p, TOKEN_NOT)) {
        ASTExpr *operand = parse_unary(p);
        if (!operand) return NULL;
        return ast_expr_unary(EXPR_NOT, operand);
    }
    
    return parse_primary(p);
}

static ASTExpr *parse_factor(Parser *p) {
    ASTExpr *left = parse_unary(p);
    if (!left) return NULL;
    
    while (parser_match(p, TOKEN_STAR) || parser_match(p, TOKEN_SLASH) ||
           parser_match(p, TOKEN_PERCENT)) {
        Token *op = parser_previous(p);
        ASTExpr *right = parse_unary(p);
        if (!right) {
            ast_expr_destroy(left);
            return NULL;
        }
        
        ExprKind kind;
        switch (op->kind) {
            case TOKEN_STAR:    kind = EXPR_MUL; break;
            case TOKEN_SLASH:   kind = EXPR_DIV; break;
            case TOKEN_PERCENT: kind = EXPR_MOD; break;
            default:            kind = EXPR_MUL; break; /* Shouldn't happen */
        }
        
        left = ast_expr_binary(kind, left, right);
        if (!left) {
            ast_expr_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

static ASTExpr *parse_term(Parser *p) {
    ASTExpr *left = parse_factor(p);
    if (!left) return NULL;
    
    while (parser_match(p, TOKEN_PLUS) || parser_match(p, TOKEN_MINUS)) {
        Token *op = parser_previous(p);
        ASTExpr *right = parse_factor(p);
        if (!right) {
            ast_expr_destroy(left);
            return NULL;
        }
        
        ExprKind kind = (op->kind == TOKEN_PLUS) ? EXPR_ADD : EXPR_SUB;
        left = ast_expr_binary(kind, left, right);
        if (!left) {
            ast_expr_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

static ASTExpr *parse_comparison(Parser *p) {
    ASTExpr *left = parse_term(p);
    if (!left) return NULL;
    
    while (parser_match(p, TOKEN_LT) || parser_match(p, TOKEN_LE) ||
           parser_match(p, TOKEN_GT) || parser_match(p, TOKEN_GE)) {
        Token *op = parser_previous(p);
        ASTExpr *right = parse_term(p);
        if (!right) {
            ast_expr_destroy(left);
            return NULL;
        }
        
        ExprKind kind;
        switch (op->kind) {
            case TOKEN_LT: kind = EXPR_LT; break;
            case TOKEN_LE: kind = EXPR_LE; break;
            case TOKEN_GT: kind = EXPR_GT; break;
            case TOKEN_GE: kind = EXPR_GE; break;
            default:       kind = EXPR_LT; break;
        }
        
        left = ast_expr_binary(kind, left, right);
        if (!left) {
            ast_expr_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

static ASTExpr *parse_equality(Parser *p) {
    ASTExpr *left = parse_comparison(p);
    if (!left) return NULL;
    
    while (parser_match(p, TOKEN_EQ) || parser_match(p, TOKEN_NE)) {
        Token *op = parser_previous(p);
        ASTExpr *right = parse_comparison(p);
        if (!right) {
            ast_expr_destroy(left);
            return NULL;
        }
        
        ExprKind kind = (op->kind == TOKEN_EQ) ? EXPR_EQ : EXPR_NE;
        left = ast_expr_binary(kind, left, right);
        if (!left) {
            ast_expr_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

static ASTExpr *parse_logical_and(Parser *p) {
    ASTExpr *left = parse_equality(p);
    if (!left) return NULL;
    
    while (parser_match(p, TOKEN_AND)) {
        ASTExpr *right = parse_equality(p);
        if (!right) {
            ast_expr_destroy(left);
            return NULL;
        }
        
        left = ast_expr_binary(EXPR_AND, left, right);
        if (!left) {
            ast_expr_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

static ASTExpr *parse_logical_or(Parser *p) {
    ASTExpr *left = parse_logical_and(p);
    if (!left) return NULL;
    
    while (parser_match(p, TOKEN_OR)) {
        ASTExpr *right = parse_logical_and(p);
        if (!right) {
            ast_expr_destroy(left);
            return NULL;
        }
        
        left = ast_expr_binary(EXPR_OR, left, right);
        if (!left) {
            ast_expr_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

static ASTExpr *parse_expression(Parser *p) {
    return parse_logical_or(p);
}

/* ==============================================================================
 * Statement Parsing
 * ==============================================================================
 */

static Type parse_type(Parser *p) {
    if (parser_match(p, TOKEN_INT)) {
        return type_int();
    }
    if (parser_match(p, TOKEN_BOOL)) {
        return type_bool();
    }
    
    Token *tok = parser_current(p);
    if (tok) {
        snprintf(p->error_msg, sizeof(p->error_msg),
                "Expected type at line %zu, column %zu",
                tok->line, tok->column);
    } else {
        snprintf(p->error_msg, sizeof(p->error_msg),
                "Expected type but reached end of file");
    }
    p->has_error = true;
    return type_int(); /* Return dummy */
}

static ASTStmt *parse_var_decl(Parser *p) {
    Token *name_tok = parser_expect(p, TOKEN_IDENTIFIER, "Expected variable name");
    if (!name_tok) return NULL;
    
    if (!parser_expect(p, TOKEN_ASSIGN, "Expected '=' after variable name")) {
        return NULL;
    }
    
    ASTExpr *init = parse_expression(p);
    if (!init) return NULL;
    
    if (!parser_expect(p, TOKEN_SEMICOLON, "Expected ';' after variable declaration")) {
        ast_expr_destroy(init);
        return NULL;
    }
    
    /* Infer type from initializer (will be validated by type checker) */
    return ast_stmt_var_decl(name_tok->lexeme, type_int(), init);
}

static ASTStmt *parse_if_statement(Parser *p) {
    if (!parser_expect(p, TOKEN_LPAREN, "Expected '(' after 'if'")) {
        return NULL;
    }
    
    ASTExpr *condition = parse_expression(p);
    if (!condition) return NULL;
    
    if (!parser_expect(p, TOKEN_RPAREN, "Expected ')' after condition")) {
        ast_expr_destroy(condition);
        return NULL;
    }
    
    ASTStmt *then_block = parse_block(p);
    if (!then_block) {
        ast_expr_destroy(condition);
        return NULL;
    }
    
    ASTStmt *else_block = NULL;
    if (parser_match(p, TOKEN_ELSE)) {
        else_block = parse_block(p);
        if (!else_block) {
            ast_expr_destroy(condition);
            ast_stmt_destroy(then_block);
            return NULL;
        }
    }
    
    return ast_stmt_if(condition, then_block, else_block);
}

static ASTStmt *parse_while_statement(Parser *p) {
    if (!parser_expect(p, TOKEN_LPAREN, "Expected '(' after 'while'")) {
        return NULL;
    }
    
    ASTExpr *condition = parse_expression(p);
    if (!condition) return NULL;
    
    if (!parser_expect(p, TOKEN_RPAREN, "Expected ')' after condition")) {
        ast_expr_destroy(condition);
        return NULL;
    }
    
    ASTStmt *body = parse_block(p);
    if (!body) {
        ast_expr_destroy(condition);
        return NULL;
    }
    
    return ast_stmt_while(condition, body);
}

static ASTStmt *parse_return_statement(Parser *p) {
    ASTExpr *expr = NULL;
    
    if (!parser_check(p, TOKEN_SEMICOLON)) {
        expr = parse_expression(p);
        if (!expr) return NULL;
    }
    
    if (!parser_expect(p, TOKEN_SEMICOLON, "Expected ';' after return")) {
        ast_expr_destroy(expr);
        return NULL;
    }
    
    return ast_stmt_return(expr);
}

static ASTStmt *parse_statement(Parser *p) {
    /* Variable declaration */
    if (parser_match(p, TOKEN_VAR)) {
        return parse_var_decl(p);
    }
    
    /* If statement */
    if (parser_match(p, TOKEN_IF)) {
        return parse_if_statement(p);
    }
    
    /* While statement */
    if (parser_match(p, TOKEN_WHILE)) {
        return parse_while_statement(p);
    }
    
    /* Return statement */
    if (parser_match(p, TOKEN_RETURN)) {
        return parse_return_statement(p);
    }
    
    /* Block */
    if (parser_check(p, TOKEN_LBRACE)) {
        return parse_block(p);
    }
    
    /* Assignment or expression statement */
    size_t checkpoint = p->current;
    if (parser_check(p, TOKEN_IDENTIFIER)) {
        Token *name_tok = parser_advance(p);
        
        if (parser_match(p, TOKEN_ASSIGN)) {
            /* Assignment */
            ASTExpr *expr = parse_expression(p);
            if (!expr) return NULL;
            
            if (!parser_expect(p, TOKEN_SEMICOLON, "Expected ';' after assignment")) {
                ast_expr_destroy(expr);
                return NULL;
            }
            
            return ast_stmt_assign(name_tok->lexeme, expr);
        }
        
        /* Not an assignment, backtrack and parse as expression */
        p->current = checkpoint;
    }
    
    /* Expression statement */
    ASTExpr *expr = parse_expression(p);
    if (!expr) return NULL;
    
    if (!parser_expect(p, TOKEN_SEMICOLON, "Expected ';' after expression")) {
        ast_expr_destroy(expr);
        return NULL;
    }
    
    return ast_stmt_expr(expr);
}

static ASTStmt *parse_block(Parser *p) {
    if (!parser_expect(p, TOKEN_LBRACE, "Expected '{'")) {
        return NULL;
    }
    
    ASTStmt **statements = NULL;
    size_t stmt_count = 0;
    size_t stmt_capacity = 0;
    
    while (!parser_check(p, TOKEN_RBRACE) && !parser_is_at_end(p)) {
        ASTStmt *stmt = parse_statement(p);
        if (!stmt) {
            /* Clean up on error */
            for (size_t i = 0; i < stmt_count; i++) {
                ast_stmt_destroy(statements[i]);
            }
            free(statements);
            return NULL;
        }
        
        /* Grow array if needed */
        if (stmt_count >= stmt_capacity) {
            size_t new_capacity = stmt_capacity == 0 ? 4 : stmt_capacity * 2;
            ASTStmt **new_stmts = realloc(statements, new_capacity * sizeof(ASTStmt*));
            if (!new_stmts) {
                ast_stmt_destroy(stmt);
                for (size_t i = 0; i < stmt_count; i++) {
                    ast_stmt_destroy(statements[i]);
                }
                free(statements);
                snprintf(p->error_msg, sizeof(p->error_msg), "Out of memory");
                p->has_error = true;
                return NULL;
            }
            statements = new_stmts;
            stmt_capacity = new_capacity;
        }
        
        statements[stmt_count++] = stmt;
    }
    
    if (!parser_expect(p, TOKEN_RBRACE, "Expected '}'")) {
        for (size_t i = 0; i < stmt_count; i++) {
            ast_stmt_destroy(statements[i]);
        }
        free(statements);
        return NULL;
    }
    
    return ast_stmt_block(statements, stmt_count);
}

/* ==============================================================================
 * Function Parsing
 * ==============================================================================
 */

static ASTFunc *parse_function(Parser *p) {
    if (!parser_expect(p, TOKEN_FUNC, "Expected 'func'")) {
        return NULL;
    }
    
    Token *name_tok = parser_expect(p, TOKEN_IDENTIFIER, "Expected function name");
    if (!name_tok) return NULL;
    
    if (!parser_expect(p, TOKEN_LPAREN, "Expected '(' after function name")) {
        return NULL;
    }
    
    /* Parse parameters */
    Param *params = NULL;
    size_t param_count = 0;
    size_t param_capacity = 0;
    
    if (!parser_check(p, TOKEN_RPAREN)) {
        do {
            Token *param_name = parser_expect(p, TOKEN_IDENTIFIER, "Expected parameter name");
            if (!param_name) {
                for (size_t i = 0; i < param_count; i++) {
                    free(params[i].name);
                }
                free(params);
                return NULL;
            }
            
            if (!parser_expect(p, TOKEN_COLON, "Expected ':' after parameter name")) {
                for (size_t i = 0; i < param_count; i++) {
                    free(params[i].name);
                }
                free(params);
                return NULL;
            }
            
            Type param_type = parse_type(p);
            if (p->has_error) {
                for (size_t i = 0; i < param_count; i++) {
                    free(params[i].name);
                }
                free(params);
                return NULL;
            }
            
            /* Grow array if needed */
            if (param_count >= param_capacity) {
                size_t new_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                Param *new_params = realloc(params, new_capacity * sizeof(Param));
                if (!new_params) {
                    for (size_t i = 0; i < param_count; i++) {
                        free(params[i].name);
                    }
                    free(params);
                    snprintf(p->error_msg, sizeof(p->error_msg), "Out of memory");
                    p->has_error = true;
                    return NULL;
                }
                params = new_params;
                param_capacity = new_capacity;
            }
            
            params[param_count].name = strdup(param_name->lexeme);
            params[param_count].type = param_type;
            param_count++;
            
        } while (parser_match(p, TOKEN_COMMA));
    }
    
    if (!parser_expect(p, TOKEN_RPAREN, "Expected ')' after parameters")) {
        for (size_t i = 0; i < param_count; i++) {
            free(params[i].name);
        }
        free(params);
        return NULL;
    }
    
    if (!parser_expect(p, TOKEN_COLON, "Expected ':' before return type")) {
        for (size_t i = 0; i < param_count; i++) {
            free(params[i].name);
        }
        free(params);
        return NULL;
    }
    
    Type return_type = parse_type(p);
    if (p->has_error) {
        for (size_t i = 0; i < param_count; i++) {
            free(params[i].name);
        }
        free(params);
        return NULL;
    }
    
    ASTStmt *body = parse_block(p);
    if (!body) {
        for (size_t i = 0; i < param_count; i++) {
            free(params[i].name);
        }
        free(params);
        return NULL;
    }
    
    return ast_func_create(name_tok->lexeme, params, param_count, return_type, body);
}

/* ==============================================================================
 * Program Parsing
 * ==============================================================================
 */

static ASTProgram *parse_program(Parser *p) {
    ASTFunc **functions = NULL;
    size_t func_count = 0;
    size_t func_capacity = 0;
    
    while (!parser_is_at_end(p)) {
        ASTFunc *func = parse_function(p);
        if (!func) {
            /* Clean up on error */
            for (size_t i = 0; i < func_count; i++) {
                ast_func_destroy(functions[i]);
            }
            free(functions);
            return NULL;
        }
        
        /* Grow array if needed */
        if (func_count >= func_capacity) {
            size_t new_capacity = func_capacity == 0 ? 4 : func_capacity * 2;
            ASTFunc **new_funcs = realloc(functions, new_capacity * sizeof(ASTFunc*));
            if (!new_funcs) {
                ast_func_destroy(func);
                for (size_t i = 0; i < func_count; i++) {
                    ast_func_destroy(functions[i]);
                }
                free(functions);
                snprintf(p->error_msg, sizeof(p->error_msg), "Out of memory");
                p->has_error = true;
                return NULL;
            }
            functions = new_funcs;
            func_capacity = new_capacity;
        }
        
        functions[func_count++] = func;
    }
    
    if (func_count == 0) {
        snprintf(p->error_msg, sizeof(p->error_msg),
                "Program must contain at least one function");
        p->has_error = true;
        return NULL;
    }
    
    return ast_program_create(functions, func_count);
}

/* ==============================================================================
 * Public Parser API
 * ==============================================================================
 */

ASTProgram *parse_tokens(TokenList *tokens, char *error_msg, size_t error_msg_size) {
    if (!tokens || tokens->count == 0) {
        if (error_msg) {
            snprintf(error_msg, error_msg_size, "No tokens to parse");
        }
        return NULL;
    }
    
    Parser parser = {
        .tokens = tokens->tokens,
        .count = tokens->count,
        .current = 0,
        .has_error = false
    };
    parser.error_msg[0] = '\0';
    
    ASTProgram *program = parse_program(&parser);
    
    if (parser.has_error && error_msg) {
        snprintf(error_msg, error_msg_size, "%s", parser.error_msg);
    }
    
    return program;
}

/* ==============================================================================
 * Parser Event (EventChains Integration)
 * ==============================================================================
 */

EventResult compiler_parser_event(EventContext *context, void *user_data) {
    (void)user_data;
    
    EventResult result;
    
    /* Get tokens from context */
    TokenList *tokens;
    EventChainErrorCode err = event_context_get(context, "tokens", (void**)&tokens);
    
    if (err != EC_SUCCESS || !tokens) {
        event_result_failure(&result, "No tokens provided to parser",
                           EC_ERROR_INVALID_PARAMETER, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Parse tokens into AST */
    char error_msg[1024];
    ASTProgram *program = parse_tokens(tokens, error_msg, sizeof(error_msg));
    
    if (!program) {
        char full_msg[1100];
        snprintf(full_msg, sizeof(full_msg), "Parser failed: %s", error_msg);
        event_result_failure(&result, full_msg,
                           EC_ERROR_INVALID_PARAMETER, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Store AST in context */
    err = event_context_set_with_cleanup(context, "ast", program,
                                         (ValueCleanupFunc)ast_program_destroy);
    
    if (err != EC_SUCCESS) {
        ast_program_destroy(program);
        event_result_failure(&result, "Failed to store AST in context",
                           err, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Success */
    event_result_success(&result);
    return result;
}
