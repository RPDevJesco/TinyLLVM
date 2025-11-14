/**
 * ==============================================================================
 * TinyLLVM Full Compiler Pipeline Test
 * ==============================================================================
 * 
 * Tests the complete compilation pipeline:
 * Source Code → Lexer → Parser → Type Checker → Code Generator → C Code
 */

#include "include/tinyllvm_compiler.h"
#include "include/eventchains.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_separator(const char *title) {
    printf("\n");
    printf("================================================================\n");
    printf("%s\n", title);
    printf("================================================================\n\n");
}

int main(void) {
    printf("=== TinyLLVM Full Compiler Pipeline Test ===\n");
    
    /* Initialize EventChains */
    event_chain_initialize();
    
    /* Source program - factorial */
    const char *source = 
        "func factorial(n: int) : int {\n"
        "    var result = 1;\n"
        "    while (n > 1) {\n"
        "        result = result * n;\n"
        "        n = n - 1;\n"
        "    }\n"
        "    return result;\n"
        "}\n"
        "\n"
        "func main() : int {\n"
        "    var x = 5;\n"
        "    var fact = factorial(x);\n"
        "    print(fact);\n"
        "    return 0;\n"
        "}\n";
    
    print_separator("Source Code");
    printf("%s\n", source);
    
    /* Create compiler configuration */
    CompilerConfig config = {
        .target = TARGET_C,
        .enable_optimization = false,
        .optimization_level = 0,
        .emit_debug_info = true,
        .emit_comments = true,
        .pretty_print = true,
        .track_memory = false,
        .max_memory_bytes = EVENTCHAINS_MAX_CONTEXT_MEMORY,
        .error_detail = ERROR_DETAIL_FULL,
        .stop_on_first_error = true
    };
    
    /* Create event chain for the full compilation pipeline */
    EventChain *chain = event_chain_create(FAULT_TOLERANCE_STRICT);
    
    /* Add compilation events */
    print_separator("Building Compiler Pipeline");
    
    ChainableEvent *lexer = chainable_event_create(
        compiler_lexer_event, NULL, "Lexer");
    printf("✓ Created Lexer event\n");
    
    ChainableEvent *parser = chainable_event_create(
        compiler_parser_event, NULL, "Parser");
    printf("✓ Created Parser event\n");
    
    ChainableEvent *typechecker = chainable_event_create(
        compiler_type_checker_event, NULL, "TypeChecker");
    printf("✓ Created Type Checker event\n");
    
    ChainableEvent *codegen = chainable_event_create(
        compiler_codegen_event, &config, "CodeGen");
    printf("✓ Created Code Generator event\n");
    
    /* Add events to chain */
    event_chain_add_event(chain, lexer);
    event_chain_add_event(chain, parser);
    event_chain_add_event(chain, typechecker);
    event_chain_add_event(chain, codegen);
    
    printf("\nPipeline: Lexer → Parser → TypeChecker → CodeGen\n");
    
    /* Set source code in context */
    EventContext *ctx = event_chain_get_context(chain);
    char *source_copy = strdup(source);
    event_context_set_with_cleanup(ctx, "source_code", source_copy, free);
    
    /* Execute the full compilation pipeline */
    print_separator("Executing Compilation Pipeline");
    
    ChainResult result;
    event_chain_execute(chain, &result);
    
    /* Check if compilation succeeded */
    if (!result.success) {
        printf("❌ Compilation FAILED!\n\n");
        
        if (result.failure_count > 0) {
            FailureInfo *failures = (FailureInfo *)result.failures;
            for (size_t i = 0; i < result.failure_count; i++) {
                printf("Error in %s:\n", failures[i].event_name);
                printf("  %s\n", failures[i].error_message);
                printf("  Error code: %d\n\n", failures[i].error_code);
            }
        }
        
        chain_result_destroy(&result);
        event_chain_destroy(chain);
        event_chain_cleanup();
        return 1;
    }
    
    printf("✓ Lexer: Tokenized source code\n");
    printf("✓ Parser: Built AST\n");
    printf("✓ Type Checker: Validated types\n");
    printf("✓ Code Generator: Generated C code\n");
    
    /* Retrieve generated code */
    char *output_code;
    EventChainErrorCode err = event_context_get(ctx, "output_code", (void**)&output_code);
    
    if (err != EC_SUCCESS || !output_code) {
        printf("\n❌ Failed to retrieve generated code from context\n");
        chain_result_destroy(&result);
        event_chain_destroy(chain);
        event_chain_cleanup();
        return 1;
    }
    
    print_separator("Generated C Code");
    printf("%s\n", output_code);
    
    print_separator("Compilation Statistics");
    
    /* Get tokens for stats */
    TokenList *tokens;
    event_context_get(ctx, "tokens", (void**)&tokens);
    if (tokens) {
        printf("Tokens parsed: %zu\n", tokens->count);
    }
    
    /* Get AST for stats */
    ASTProgram *program;
    event_context_get(ctx, "ast", (void**)&program);
    if (program) {
        printf("Functions defined: %zu\n", program->func_count);
    }
    
    printf("Generated code length: %zu bytes\n", strlen(output_code));
    printf("Target: C\n");
    
    print_separator("Test Result");
    printf("✅ COMPILATION SUCCESSFUL!\n");
    printf("\nThe generated C code can be compiled with:\n");
    printf("  gcc -o program output.c\n");
    printf("  ./program\n");
    printf("\nExpected output: 120 (factorial of 5)\n");
    
    /* Cleanup */
    chain_result_destroy(&result);
    event_chain_destroy(chain);
    event_chain_cleanup();
    
    printf("\n=== Full Compiler Pipeline Test Complete ===\n");
    
    return 0;
}
