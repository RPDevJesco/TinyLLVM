/**
 * ==============================================================================
 * TinyLLVM Compiler - Lexer Test with EventChains
 * ==============================================================================
 */

#include "include/tinyllvm_compiler.h"
#include "include/eventchains.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_tokens(TokenList *tokens) {
    printf("Tokens (%zu):\n", tokens->count);
    printf("%-20s %-15s %10s %10s\n", "Kind", "Lexeme", "Line", "Column");
    printf("----------------------------------------------------------------\n");
    
    for (size_t i = 0; i < tokens->count; i++) {
        Token *tok = tokens->tokens[i];
        const char *lexeme = tok->lexeme ? tok->lexeme : "";
        
        printf("%-20s %-15s %10zu %10zu",
               token_kind_to_string(tok->kind),
               lexeme,
               tok->line,
               tok->column);
        
        if (tok->kind == TOKEN_INT_LITERAL) {
            printf("  (value: %d)", tok->value);
        }
        
        printf("\n");
    }
}

int main(void) {
    fprintf(stderr, "START\n");
    fflush(stderr);
    
    const char *msg = "TinyLLVM Lexer Test\n";
    fprintf(stderr, "%s", msg);
    fflush(stderr);
    
    /* Initialize EventChains */
    fprintf(stderr, "About to init EventChains\n");
    fflush(stderr);
    event_chain_initialize();
    fprintf(stderr, "EventChains initialized\n");
    fflush(stderr);
    
    /* Sample program */
    const char *source = 
        "func factorial(n: int) : int {\n"
        "    var result = 1;\n"
        "    while (n > 1) {\n"
        "        result = result * n;\n"
        "        n = n - 1;\n"
        "    }\n"
        "    return result;\n"
        "}\n";
    
    printf("Source Code:\n");
    printf("------------\n%s\n", source);
    
    /* Create event chain for just the lexer */
    printf("DEBUG: Creating event chain...\n");
    EventChain *chain = event_chain_create(FAULT_TOLERANCE_STRICT);
    printf("DEBUG: Event chain created: %p\n", (void*)chain);
    
    /* Add lexer event */
    printf("DEBUG: Creating lexer event...\n");
    ChainableEvent *lexer_event = chainable_event_create(
        compiler_lexer_event,
        NULL,
        "Lexer"
    );
    
    if (!lexer_event) {
        fprintf(stderr, "Error: Failed to create lexer event\n");
        event_chain_destroy(chain);
        return 1;
    }
    
    EventChainErrorCode err = event_chain_add_event(chain, lexer_event);
    if (err != EC_SUCCESS) {
        fprintf(stderr, "Error: Failed to add lexer event: %s\n",
                event_chain_error_string(err));
        event_chain_destroy(chain);
        return 1;
    }
    
    /* Set source code in context */
    EventContext *ctx = event_chain_get_context(chain);
    char *source_copy = malloc(strlen(source) + 1);
    if (!source_copy) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        event_chain_destroy(chain);
        return 1;
    }
    strcpy(source_copy, source);
    
    err = event_context_set_with_cleanup(ctx, "source_code", source_copy, free);
    if (err != EC_SUCCESS) {
        fprintf(stderr, "Error: Failed to set source code: %s\n",
                event_chain_error_string(err));
        free(source_copy);
        event_chain_destroy(chain);
        return 1;
    }
    
    /* Execute the chain */
    printf("\nExecuting Lexer via EventChains...\n\n");
    
    ChainResult result;
    event_chain_execute(chain, &result);
    
    if (!result.success) {
        fprintf(stderr, "Lexer failed!\n");
        
        if (result.failure_count > 0) {
            FailureInfo *failures = (FailureInfo *)result.failures;
            for (size_t i = 0; i < result.failure_count; i++) {
                fprintf(stderr, "  Event '%s': %s (code %d)\n",
                       failures[i].event_name,
                       failures[i].error_message,
                       failures[i].error_code);
            }
        }
        
        chain_result_destroy(&result);
        event_chain_destroy(chain);
        return 1;
    }
    
    printf("Lexer succeeded!\n\n");
    
    /* Retrieve tokens from context */
    TokenList *tokens;
    err = event_context_get(ctx, "tokens", (void**)&tokens);
    
    if (err != EC_SUCCESS || !tokens) {
        fprintf(stderr, "Error: Failed to retrieve tokens from context\n");
        chain_result_destroy(&result);
        event_chain_destroy(chain);
        return 1;
    }
    
    /* Print tokens */
    print_tokens(tokens);
    
    /* Cleanup */
    chain_result_destroy(&result);
    event_chain_destroy(chain);
    event_chain_cleanup();
    
    printf("\n=== Lexer Test Complete ===\n");
    
    return 0;
}
