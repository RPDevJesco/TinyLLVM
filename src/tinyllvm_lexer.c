/**
 * ==============================================================================
 * TinyLLVM Compiler - Lexer Implementation
 * ==============================================================================
 */

#include "include/tinyllvm_compiler.h"
#include "include/eventchains.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==============================================================================
 * Token Management
 * ==============================================================================
 */

const char *token_kind_to_string(TokenKind kind) {
    switch (kind) {
        case TOKEN_FUNC:        return "func";
        case TOKEN_VAR:         return "var";
        case TOKEN_IF:          return "if";
        case TOKEN_ELSE:        return "else";
        case TOKEN_WHILE:       return "while";
        case TOKEN_RETURN:      return "return";
        case TOKEN_TRUE:        return "true";
        case TOKEN_FALSE:       return "false";
        case TOKEN_INT:         return "int";
        case TOKEN_BOOL:        return "bool";
        case TOKEN_IDENTIFIER:  return "IDENTIFIER";
        case TOKEN_INT_LITERAL: return "INT_LITERAL";
        case TOKEN_PLUS:        return "+";
        case TOKEN_MINUS:       return "-";
        case TOKEN_STAR:        return "*";
        case TOKEN_SLASH:       return "/";
        case TOKEN_PERCENT:     return "%";
        case TOKEN_EQ:          return "==";
        case TOKEN_NE:          return "!=";
        case TOKEN_LT:          return "<";
        case TOKEN_LE:          return "<=";
        case TOKEN_GT:          return ">";
        case TOKEN_GE:          return ">=";
        case TOKEN_AND:         return "&&";
        case TOKEN_OR:          return "||";
        case TOKEN_NOT:         return "!";
        case TOKEN_ASSIGN:      return "=";
        case TOKEN_SEMICOLON:   return ";";
        case TOKEN_COLON:       return ":";
        case TOKEN_COMMA:       return ",";
        case TOKEN_LPAREN:      return "(";
        case TOKEN_RPAREN:      return ")";
        case TOKEN_LBRACE:      return "{";
        case TOKEN_RBRACE:      return "}";
        case TOKEN_EOF:         return "EOF";
        case TOKEN_ERROR:       return "ERROR";
        default:                return "UNKNOWN";
    }
}

static Token *token_create(TokenKind kind, const char *lexeme, size_t length,
                          size_t line, size_t column) {
    Token *token = malloc(sizeof(Token));
    if (!token) return NULL;
    
    token->kind = kind;
    token->length = length;
    token->line = line;
    token->column = column;
    token->value = 0;
    
    if (lexeme && length > 0) {
        token->lexeme = malloc(length + 1);
        if (!token->lexeme) {
            free(token);
            return NULL;
        }
        memcpy(token->lexeme, lexeme, length);
        token->lexeme[length] = '\0';
        
        /* Parse integer literals */
        if (kind == TOKEN_INT_LITERAL) {
            token->value = atoi(token->lexeme);
        }
    } else {
        token->lexeme = NULL;
    }
    
    return token;
}

static void token_destroy(Token *token) {
    if (!token) return;
    free(token->lexeme);
    free(token);
}

void token_list_destroy(TokenList *tokens) {
    if (!tokens) return;
    
    for (size_t i = 0; i < tokens->count; i++) {
        token_destroy(tokens->tokens[i]);
    }
    free(tokens->tokens);
    free(tokens);
}

/* ==============================================================================
 * Lexer State
 * ==============================================================================
 */

typedef struct {
    const char *source;
    size_t length;
    size_t current;
    size_t line;
    size_t column;
    
    TokenList *tokens;
} Lexer;

static bool lexer_is_at_end(Lexer *lex) {
    return lex->current >= lex->length;
}

static char lexer_peek(Lexer *lex) {
    if (lexer_is_at_end(lex)) return '\0';
    return lex->source[lex->current];
}

static char lexer_peek_next(Lexer *lex) {
    if (lex->current + 1 >= lex->length) return '\0';
    return lex->source[lex->current + 1];
}

static char lexer_advance(Lexer *lex) {
    if (lexer_is_at_end(lex)) return '\0';
    
    char c = lex->source[lex->current++];
    
    if (c == '\n') {
        lex->line++;
        lex->column = 0;
    } else {
        lex->column++;
    }
    
    return c;
}

static bool lexer_match(Lexer *lex, char expected) {
    if (lexer_is_at_end(lex)) return false;
    if (lex->source[lex->current] != expected) return false;
    
    lexer_advance(lex);
    return true;
}

static void lexer_add_token(Lexer *lex, TokenKind kind, const char *lexeme,
                           size_t length, size_t line, size_t column) {
    /* Grow array if needed */
    if (lex->tokens->count >= lex->tokens->capacity) {
        size_t new_capacity = lex->tokens->capacity == 0 ? 
                             32 : lex->tokens->capacity * 2;
        Token **new_tokens = realloc(lex->tokens->tokens, 
                                     new_capacity * sizeof(Token*));
        if (!new_tokens) return;
        
        lex->tokens->tokens = new_tokens;
        lex->tokens->capacity = new_capacity;
    }
    
    Token *token = token_create(kind, lexeme, length, line, column);
    if (token) {
        lex->tokens->tokens[lex->tokens->count++] = token;
    }
}

/* ==============================================================================
 * Lexer - Character Classification
 * ==============================================================================
 */

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static void skip_whitespace(Lexer *lex) {
    while (!lexer_is_at_end(lex)) {
        char c = lexer_peek(lex);
        
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            lexer_advance(lex);
        } else if (c == '/' && lexer_peek_next(lex) == '/') {
            /* Single-line comment */
            while (!lexer_is_at_end(lex) && lexer_peek(lex) != '\n') {
                lexer_advance(lex);
            }
        } else if (c == '/' && lexer_peek_next(lex) == '*') {
            /* Multi-line comment */
            lexer_advance(lex);  /* / */
            lexer_advance(lex);  /* * */
            
            while (!lexer_is_at_end(lex)) {
                if (lexer_peek(lex) == '*' && lexer_peek_next(lex) == '/') {
                    lexer_advance(lex);  /* * */
                    lexer_advance(lex);  /* / */
                    break;
                }
                lexer_advance(lex);
            }
        } else {
            break;
        }
    }
}

/* ==============================================================================
 * Lexer - Token Scanning
 * ==============================================================================
 */

static TokenKind check_keyword(const char *lexeme, size_t length,
                               const char *keyword, TokenKind kind) {
    if (strlen(keyword) == length && memcmp(lexeme, keyword, length) == 0) {
        return kind;
    }
    return TOKEN_IDENTIFIER;
}

static TokenKind identifier_type(const char *lexeme, size_t length) {
    /* Check keywords */
    switch (lexeme[0]) {
        case 'b':
            return check_keyword(lexeme, length, "bool", TOKEN_BOOL);
        case 'e':
            return check_keyword(lexeme, length, "else", TOKEN_ELSE);
        case 'f':
            if (length > 1) {
                if (lexeme[1] == 'u') return check_keyword(lexeme, length, "func", TOKEN_FUNC);
                if (lexeme[1] == 'a') return check_keyword(lexeme, length, "false", TOKEN_FALSE);
            }
            break;
        case 'i':
            if (length > 1 && lexeme[1] == 'f') {
                return check_keyword(lexeme, length, "if", TOKEN_IF);
            }
            return check_keyword(lexeme, length, "int", TOKEN_INT);
        case 'r':
            return check_keyword(lexeme, length, "return", TOKEN_RETURN);
        case 't':
            return check_keyword(lexeme, length, "true", TOKEN_TRUE);
        case 'v':
            return check_keyword(lexeme, length, "var", TOKEN_VAR);
        case 'w':
            return check_keyword(lexeme, length, "while", TOKEN_WHILE);
    }
    
    return TOKEN_IDENTIFIER;
}

static void scan_identifier(Lexer *lex) {
    size_t start = lex->current - 1;
    size_t start_column = lex->column - 1;
    
    while (is_alnum(lexer_peek(lex))) {
        lexer_advance(lex);
    }
    
    size_t length = lex->current - start;
    const char *lexeme = lex->source + start;
    
    TokenKind kind = identifier_type(lexeme, length);
    lexer_add_token(lex, kind, lexeme, length, lex->line, start_column);
}

static void scan_number(Lexer *lex) {
    size_t start = lex->current - 1;
    size_t start_column = lex->column - 1;
    
    while (is_digit(lexer_peek(lex))) {
        lexer_advance(lex);
    }
    
    size_t length = lex->current - start;
    const char *lexeme = lex->source + start;
    
    lexer_add_token(lex, TOKEN_INT_LITERAL, lexeme, length, lex->line, start_column);
}

static void scan_token(Lexer *lex) {
    skip_whitespace(lex);
    
    if (lexer_is_at_end(lex)) {
        lexer_add_token(lex, TOKEN_EOF, NULL, 0, lex->line, lex->column);
        return;
    }
    
    size_t start_column = lex->column;
    char c = lexer_advance(lex);
    
    if (is_alpha(c)) {
        scan_identifier(lex);
        return;
    }
    
    if (is_digit(c)) {
        scan_number(lex);
        return;
    }
    
    switch (c) {
        case '(': lexer_add_token(lex, TOKEN_LPAREN, "(", 1, lex->line, start_column); break;
        case ')': lexer_add_token(lex, TOKEN_RPAREN, ")", 1, lex->line, start_column); break;
        case '{': lexer_add_token(lex, TOKEN_LBRACE, "{", 1, lex->line, start_column); break;
        case '}': lexer_add_token(lex, TOKEN_RBRACE, "}", 1, lex->line, start_column); break;
        case ';': lexer_add_token(lex, TOKEN_SEMICOLON, ";", 1, lex->line, start_column); break;
        case ':': lexer_add_token(lex, TOKEN_COLON, ":", 1, lex->line, start_column); break;
        case ',': lexer_add_token(lex, TOKEN_COMMA, ",", 1, lex->line, start_column); break;
        case '+': lexer_add_token(lex, TOKEN_PLUS, "+", 1, lex->line, start_column); break;
        case '-': lexer_add_token(lex, TOKEN_MINUS, "-", 1, lex->line, start_column); break;
        case '*': lexer_add_token(lex, TOKEN_STAR, "*", 1, lex->line, start_column); break;
        case '/': lexer_add_token(lex, TOKEN_SLASH, "/", 1, lex->line, start_column); break;
        case '%': lexer_add_token(lex, TOKEN_PERCENT, "%", 1, lex->line, start_column); break;
        
        case '=':
            if (lexer_match(lex, '=')) {
                lexer_add_token(lex, TOKEN_EQ, "==", 2, lex->line, start_column);
            } else {
                lexer_add_token(lex, TOKEN_ASSIGN, "=", 1, lex->line, start_column);
            }
            break;
            
        case '!':
            if (lexer_match(lex, '=')) {
                lexer_add_token(lex, TOKEN_NE, "!=", 2, lex->line, start_column);
            } else {
                lexer_add_token(lex, TOKEN_NOT, "!", 1, lex->line, start_column);
            }
            break;
            
        case '<':
            if (lexer_match(lex, '=')) {
                lexer_add_token(lex, TOKEN_LE, "<=", 2, lex->line, start_column);
            } else {
                lexer_add_token(lex, TOKEN_LT, "<", 1, lex->line, start_column);
            }
            break;
            
        case '>':
            if (lexer_match(lex, '=')) {
                lexer_add_token(lex, TOKEN_GE, ">=", 2, lex->line, start_column);
            } else {
                lexer_add_token(lex, TOKEN_GT, ">", 1, lex->line, start_column);
            }
            break;
            
        case '&':
            if (lexer_match(lex, '&')) {
                lexer_add_token(lex, TOKEN_AND, "&&", 2, lex->line, start_column);
            } else {
                lexer_add_token(lex, TOKEN_ERROR, "&", 1, lex->line, start_column);
            }
            break;
            
        case '|':
            if (lexer_match(lex, '|')) {
                lexer_add_token(lex, TOKEN_OR, "||", 2, lex->line, start_column);
            } else {
                lexer_add_token(lex, TOKEN_ERROR, "|", 1, lex->line, start_column);
            }
            break;
            
        default:
            lexer_add_token(lex, TOKEN_ERROR, &c, 1, lex->line, start_column);
            break;
    }
}

/* ==============================================================================
 * Public Lexer API
 * ==============================================================================
 */

TokenList *lex_source(const char *source_code) {
    if (!source_code) return NULL;
    
    Lexer lex = {
        .source = source_code,
        .length = strlen(source_code),
        .current = 0,
        .line = 1,
        .column = 0,
        .tokens = NULL
    };
    
    lex.tokens = malloc(sizeof(TokenList));
    if (!lex.tokens) return NULL;
    
    lex.tokens->tokens = NULL;
    lex.tokens->count = 0;
    lex.tokens->capacity = 0;
    
    while (!lexer_is_at_end(&lex)) {
        scan_token(&lex);
        
        /* Stop if we hit EOF */
        if (lex.tokens->count > 0 &&
            lex.tokens->tokens[lex.tokens->count - 1]->kind == TOKEN_EOF) {
            break;
        }
    }
    
    /* Ensure we have EOF token */
    if (lex.tokens->count == 0 ||
        lex.tokens->tokens[lex.tokens->count - 1]->kind != TOKEN_EOF) {
        lexer_add_token(&lex, TOKEN_EOF, NULL, 0, lex.line, lex.column);
    }
    
    return lex.tokens;
}

/* ==============================================================================
 * Lexer Event (EventChains Integration)
 * ==============================================================================
 */

EventResult compiler_lexer_event(EventContext *context, void *user_data) {
    (void)user_data;
    
    EventResult result;
    
    /* Get source code from context */
    char *source_code;
    EventChainErrorCode err = event_context_get(context, "source_code",
                                                 (void**)&source_code);
    
    if (err != EC_SUCCESS || !source_code) {
        event_result_failure(&result, "No source code provided",
                           EC_ERROR_INVALID_PARAMETER, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Lex the source code */
    TokenList *tokens = lex_source(source_code);
    
    if (!tokens) {
        event_result_failure(&result, "Failed to allocate token list",
                           EC_ERROR_OUT_OF_MEMORY, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Check for lexer errors */
    for (size_t i = 0; i < tokens->count; i++) {
        if (tokens->tokens[i]->kind == TOKEN_ERROR) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Lexer error at line %zu, column %zu: unexpected character '%s'",
                    tokens->tokens[i]->line,
                    tokens->tokens[i]->column,
                    tokens->tokens[i]->lexeme);
            
            token_list_destroy(tokens);
            event_result_failure(&result, error_msg,
                               EC_ERROR_INVALID_PARAMETER, ERROR_DETAIL_FULL);
            return result;
        }
    }
    
    /* Store tokens in context */
    err = event_context_set_with_cleanup(context, "tokens", tokens,
                                         (ValueCleanupFunc)token_list_destroy);
    
    if (err != EC_SUCCESS) {
        token_list_destroy(tokens);
        event_result_failure(&result, "Failed to store tokens in context",
                           err, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Success */
    event_result_success(&result);
    return result;
}