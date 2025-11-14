/**
 * ==============================================================================
 * TinyLLVM Compiler - EventChains Integration
 * ==============================================================================
 * 
 * This is the main compiler implementation that uses EventChains as its
 * architecture. Each compilation phase is a chainable event, and middleware
 * provides cross-cutting concerns like optimization and memory management.
 * 
 * Architecture:
 *   - Lexer Event: source_code → tokens
 *   - Parser Event: tokens → AST
 *   - Type Checker Event: AST → typed AST (validates)
 *   - CodeGen Event: AST → target code (C/Rust/Go/etc.)
 * 
 * Middleware (wraps all phases):
 *   - Memory tracking
 *   - Optimization passes
 *   - Memory model management
 *   - Error handling
 * 
 * Copyright (c) 2025 TinyLLVM Project
 * Licensed under the MIT License
 * ==============================================================================
 */

#ifndef TINYLLVM_COMPILER_H
#define TINYLLVM_COMPILER_H

#include "eventchains.h"
#include "tinyllvm_ast.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==============================================================================
 * Target Language Selection
 * ==============================================================================
 */

typedef enum {
    TARGET_TINYLLVM,        /* Default: TinyLLVM IR (human-readable) */
    TARGET_C,               /* C99 code generation */
    TARGET_RUST,            /* Rust code generation */
    TARGET_GO,              /* Go code generation */
    TARGET_RUBY,            /* Ruby code generation */
    TARGET_HASKELL,         /* Haskell code generation */
    TARGET_ASM_X86_64       /* x86-64 assembly */
} CodeGenTarget;

/* ==============================================================================
 * Compiler Configuration
 * ==============================================================================
 */

typedef struct {
    /* Target language */
    CodeGenTarget target;
    
    /* Optimization settings */
    bool enable_optimization;
    int optimization_level;     /* 0-3: none, basic, moderate, aggressive */
    
    /* Code generation options */
    bool emit_debug_info;
    bool emit_comments;
    bool pretty_print;
    
    /* Memory management */
    bool track_memory;
    size_t max_memory_bytes;
    
    /* Error handling */
    ErrorDetailLevel error_detail;
    bool stop_on_first_error;
} CompilerConfig;

/* ==============================================================================
 * Compilation Result
 * ==============================================================================
 */

typedef struct {
    bool success;
    char *output_code;          /* Generated code (must be freed by caller) */
    size_t output_length;
    
    /* Statistics */
    size_t tokens_count;
    size_t ast_node_count;
    size_t memory_used;
    
    /* Errors/warnings */
    char **errors;
    size_t error_count;
    char **warnings;
    size_t warning_count;
} CompilationResult;

/* ==============================================================================
 * Lexer - Tokenization Phase
 * ==============================================================================
 */

typedef enum {
    /* Keywords */
    TOKEN_FUNC,
    TOKEN_VAR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_RETURN,
    TOKEN_TRUE,
    TOKEN_FALSE,
    
    /* Types */
    TOKEN_INT,
    TOKEN_BOOL,
    
    /* Identifiers and literals */
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    
    /* Operators */
    TOKEN_PLUS,         /* + */
    TOKEN_MINUS,        /* - */
    TOKEN_STAR,         /* * */
    TOKEN_SLASH,        /* / */
    TOKEN_PERCENT,      /* % */
    
    /* Comparisons */
    TOKEN_EQ,           /* == */
    TOKEN_NE,           /* != */
    TOKEN_LT,           /* < */
    TOKEN_LE,           /* <= */
    TOKEN_GT,           /* > */
    TOKEN_GE,           /* >= */
    
    /* Logical */
    TOKEN_AND,          /* && */
    TOKEN_OR,           /* || */
    TOKEN_NOT,          /* ! */
    
    /* Punctuation */
    TOKEN_ASSIGN,       /* = */
    TOKEN_SEMICOLON,    /* ; */
    TOKEN_COLON,        /* : */
    TOKEN_COMMA,        /* , */
    TOKEN_LPAREN,       /* ( */
    TOKEN_RPAREN,       /* ) */
    TOKEN_LBRACE,       /* { */
    TOKEN_RBRACE,       /* } */
    
    /* Special */
    TOKEN_EOF,
    TOKEN_ERROR
} TokenKind;

typedef struct {
    TokenKind kind;
    char *lexeme;               /* Actual text (owned by token) */
    size_t length;
    int value;                  /* For INT_LITERAL */
    
    /* Source location */
    size_t line;
    size_t column;
} Token;

typedef struct {
    Token **tokens;     /* Array of pointers to tokens */
    size_t count;
    size_t capacity;
} TokenList;

/* Lexer functions */
TokenList *lex_source(const char *source_code);
void token_list_destroy(TokenList *tokens);
const char *token_kind_to_string(TokenKind kind);

/* ==============================================================================
 * Compiler Pipeline Events
 * ==============================================================================
 */

/**
 * Lexer Event - Tokenizes source code
 * Input:  context["source_code"] : char*
 * Output: context["tokens"] : TokenList*
 */
EventResult compiler_lexer_event(EventContext *context, void *user_data);

/**
 * Parser Event - Builds AST from tokens
 * Input:  context["tokens"] : TokenList*
 * Output: context["ast"] : ASTProgram*
 */
EventResult compiler_parser_event(EventContext *context, void *user_data);

/**
 * Type Checker Event - Validates and annotates AST with types
 * Input:  context["ast"] : ASTProgram*
 * Output: context["ast"] : ASTProgram* (modified in-place)
 */
EventResult compiler_type_checker_event(EventContext *context, void *user_data);

/**
 * Code Generator Event - Generates target language code
 * Input:  context["ast"] : ASTProgram*
 *         user_data : CompilerConfig*
 * Output: context["output_code"] : char*
 */
EventResult compiler_codegen_event(EventContext *context, void *user_data);

/* ==============================================================================
 * Compiler Middleware
 * ==============================================================================
 */

/**
 * Memory Tracking Middleware - Tracks memory usage per phase
 */
void compiler_memory_tracking_middleware(
    EventResult *result,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
);

/**
 * Optimization Middleware - Applies optimization passes after parser
 */
void compiler_optimization_middleware(
    EventResult *result,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
);

/**
 * Memory Model Middleware - Manages arena allocators per phase
 */
void compiler_memory_model_middleware(
    EventResult *result,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
);

/* ==============================================================================
 * High-Level Compiler API
 * ==============================================================================
 */

/**
 * Create a compiler configuration with defaults
 * @return New configuration (must be freed by caller)
 */
CompilerConfig *compiler_config_create_default(void);

/**
 * Create a compiler event chain with the given configuration
 * @param config  Compiler configuration
 * @return EventChain ready to execute
 */
EventChain *compiler_create_chain(CompilerConfig *config);

/**
 * Compile source code to target language
 * @param source_code  Source code string
 * @param config       Compiler configuration
 * @param result_out   Output compilation result
 * @return EC_SUCCESS or error code
 */
EventChainErrorCode compiler_compile(
    const char *source_code,
    CompilerConfig *config,
    CompilationResult *result_out
);

/**
 * Free a compilation result
 * @param result  Result to free
 */
void compilation_result_destroy(CompilationResult *result);

/* ==============================================================================
 * Optimization Passes (Applied by Middleware)
 * ==============================================================================
 */

/**
 * Constant folding - Evaluate constant expressions at compile time
 */
void optimize_constant_folding(ASTProgram *program);

/**
 * Dead code elimination - Remove unreachable code
 */
void optimize_dead_code_elimination(ASTProgram *program);

/**
 * Common subexpression elimination
 */
void optimize_cse(ASTProgram *program);

/* ==============================================================================
 * Utility Functions
 * ==============================================================================
 */

/**
 * Get the name of a target
 */
const char *codegen_target_name(CodeGenTarget target);

/**
 * Get file extension for target
 */
const char *codegen_target_extension(CodeGenTarget target);

#ifdef __cplusplus
}
#endif

#endif /* TINYLLVM_COMPILER_H */