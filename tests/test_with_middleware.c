/**
 * ==============================================================================
 * TinyLLVM Compiler with Middleware - Complete Demonstration
 * ==============================================================================
 * 
 * Demonstrates the full power of EventChains middleware system:
 * - Logging middleware (observability)
 * - Timing middleware (performance monitoring)
 * - Memory monitor (resource tracking)
 * - Buffer overflow detector (security)
 * - Integer overflow fuzzer (robustness testing)
 * 
 * The "onion" architecture allows middleware to wrap all compilation phases!
 */

#include "include/tinyllvm_compiler.h"
#include "include/eventchains.h"

/* Include all middleware */
#include "include/logging_middleware.h"
#include "include/timing_middleware.h"
#include "include/memory_monitor_middleware.h"
#include "include/buffer_overflow_detector.h"
#include "include/integer_overflow_fuzzer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void print_separator(const char *title) {
    printf("\n");
    printf("================================================================\n");
    printf("%s\n", title);
    printf("================================================================\n\n");
}

int main(void) {
    printf("=== TinyLLVM Compiler with Middleware Stack ===\n");
    printf("Demonstrating the EventChains 'Onion' Architecture\n\n");
    
    /* Seed random for fuzzers */
    srand((unsigned int)time(NULL));
    
    /* Initialize EventChains */
    event_chain_initialize();
    
    /* Source program */
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
        .emit_comments = true,
        .pretty_print = true
    };
    
    print_separator("Building Compilation Pipeline");
    
    /* Create event chain */
    EventChain *chain = event_chain_create(FAULT_TOLERANCE_LENIENT);
    
    /* Add compilation events */
    ChainableEvent *lexer = chainable_event_create(
        compiler_lexer_event, NULL, "Lexer");
    ChainableEvent *parser = chainable_event_create(
        compiler_parser_event, NULL, "Parser");
    ChainableEvent *typechecker = chainable_event_create(
        compiler_type_checker_event, NULL, "TypeChecker");
    ChainableEvent *codegen = chainable_event_create(
        compiler_codegen_event, &config, "CodeGen");
    
    event_chain_add_event(chain, lexer);
    event_chain_add_event(chain, parser);
    event_chain_add_event(chain, typechecker);
    event_chain_add_event(chain, codegen);
    
    printf(" Created 4-phase pipeline: Lexer → Parser → TypeChecker → CodeGen\n");
    
    print_separator("Adding Middleware Stack");
    
    /* Create middleware configurations */
    BufferOverflowConfig *buffer_config = buffer_overflow_detector_create(
        false,  /* strict_mode = false (don't halt on detection) */
        true    /* use_guard_bands = true */
    );
    buffer_config->enabled = true;  /* Disable for this demo (requires special setup) */
    
    IntOverflowConfig *overflow_config = int_overflow_fuzzer_create(false);
    overflow_config->enabled = true;  /* Disable for this demo */
    
    /* Add middleware layers (in reverse order - outermost first) */
    
    /* Layer 1: Logging (outermost - sees everything first) */
    EventMiddleware *logging = event_middleware_create(
        logging_middleware, NULL, "Logging");
    event_chain_use_middleware(chain, logging);
    printf(" Added Logging middleware (Layer 1 - Outermost)\n");
    
    /* Layer 2: Timing */
    EventMiddleware *timing = event_middleware_create(
        timing_middleware, NULL, "Timing");
    event_chain_use_middleware(chain, timing);
    printf(" Added Timing middleware (Layer 2)\n");
    
    /* Layer 3: Memory monitoring */
    EventMiddleware *memory = event_middleware_create(
        memory_monitor_middleware, NULL, "MemoryMonitor");
    event_chain_use_middleware(chain, memory);
    printf(" Added Memory Monitor middleware (Layer 3)\n");
    
    /* Disabled middleware (can be enabled for testing) */
    if (buffer_config->enabled) {
        EventMiddleware *buffer = event_middleware_create(
            buffer_overflow_detector_middleware, buffer_config, "BufferOverflow");
        event_chain_use_middleware(chain, buffer);
        printf(" Added Buffer Overflow Detector (Layer 4)\n");
    } else {
        printf(" Buffer Overflow Detector (disabled)\n");
    }
    
    if (overflow_config->enabled) {
        EventMiddleware *overflow = event_middleware_create(
            integer_overflow_fuzzer_middleware, overflow_config, "IntOverflow");
        event_chain_use_middleware(chain, overflow);
        printf(" Added Integer Overflow Fuzzer (Layer 5)\n");
    } else {
        printf(" Integer Overflow Fuzzer (disabled)\n");
    }
    
    printf("\nMiddleware Stack (Onion Layers):\n");
    printf("  [Logging] → [Timing] → [Memory] → [Events] → [Memory] → [Timing] → [Logging]\n");
    printf("   ^                                                                          ^\n");
    printf("   Entry                                                                    Exit\n");
    
    /* Set source code in context */
    EventContext *ctx = event_chain_get_context(chain);
    char *source_copy = malloc(strlen(source) + 1);
    strcpy(source_copy, source);
    event_context_set_with_cleanup(ctx, "source_code", source_copy, free);
    
    print_separator("Executing Pipeline with Middleware");
    
    ChainResult result;
    event_chain_execute(chain, &result);
    
    print_separator("Execution Result");
    
    if (!result.success) {
        printf(" Compilation FAILED\n\n");
        if (result.failure_count > 0) {
            FailureInfo *failures = (FailureInfo *)result.failures;
            for (size_t i = 0; i < result.failure_count; i++) {
                printf("Error in %s:\n", failures[i].event_name);
                printf("  %s\n", failures[i].error_message);
            }
        }
    } else {
        printf(" Compilation SUCCESSFUL\n\n");
        
        /* Get generated code */
        char *output_code;
        if (event_context_get(ctx, "output_code", (void**)&output_code) == EC_SUCCESS) {
            printf("Generated C Code:\n");
            printf("-----------------\n");
            printf("%s\n", output_code);
        }
    }
    
    print_separator("Middleware Benefits Demonstrated");
    
    printf(" Logging: Full observability of pipeline execution\n");
    printf(" Timing: Performance metrics for each phase\n");
    printf(" Memory: Resource tracking and leak detection\n");
    printf(" Buffer Overflow: Security validation (disabled in demo)\n");
    printf(" Integer Overflow: Robustness testing (disabled in demo)\n");
    
    print_separator("Architecture Highlights");
    
    printf("1. Middleware Composition:\n");
    printf("   - Each middleware wraps ALL compilation phases\n");
    printf("   - 'Onion' architecture: middleware calls next() to proceed\n");
    printf("   - Can observe, modify, or reject at any point\n\n");
    
    printf("2. Zero Code Changes:\n");
    printf("   - Compiler phases unchanged\n");
    printf("   - Middleware added externally\n");
    printf("   - Perfect separation of concerns\n\n");
    
    printf("3. Extensibility:\n");
    printf("   - Add new middleware without touching core\n");
    printf("   - Enable/disable dynamically\n");
    printf("   - Configure per-compilation\n\n");
    
    printf("4. Available Middleware:\n");
    printf("    logging_middleware.h - Observability\n");
    printf("    timing_middleware.h - Performance\n");
    printf("    memory_monitor_middleware.h - Resource tracking\n");
    printf("    buffer_overflow_detector.h - Security testing\n");
    printf("    integer_overflow_fuzzer.h - Robustness testing\n");
    printf("    chaos_injection_middleware.h - Chaos engineering\n");
    printf("    context_corruptor_middleware.h - Error injection\n");
    printf("    input_fuzzer_middleware.h - Input mutation\n");
    printf("    resource_limit_middleware.h - Resource limits\n");
    printf("    use_after_free_detector.h - Memory safety\n");
    
    print_separator("Custom Middleware Example");
    
    printf("Creating custom middleware is simple:\n\n");
    printf("void my_middleware(\n");
    printf("    EventResult *result,\n");
    printf("    ChainableEvent *event,\n");
    printf("    EventContext *context,\n");
    printf("    void (*next)(...),\n");
    printf("    void *next_data,\n");
    printf("    void *user_data)\n");
    printf("{\n");
    printf("    // Before phase execution\n");
    printf("    printf(\"Before %%s\\n\", event->name);\n");
    printf("    \n");
    printf("    // Execute phase\n");
    printf("    next(result, event, context, next_data);\n");
    printf("    \n");
    printf("    // After phase execution\n");
    printf("    printf(\"After %%s\\n\", event->name);\n");
    printf("}\n");
    
    /* Cleanup */
    if (buffer_config) free(buffer_config);
    if (overflow_config) free(overflow_config);
    
    chain_result_destroy(&result);
    event_chain_destroy(chain);
    event_chain_cleanup();
    
    printf("\n=== Middleware Demonstration Complete ===\n");
    
    return 0;
}