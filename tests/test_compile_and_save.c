/**
 * ==============================================================================
 * TinyLLVM Full Compiler + Execute Test
 * ==============================================================================
 *
 * This test:
 * 1. Compiles CoreTiny source to C
 * 2. Saves the generated C code to a file
 * 3. Creates a CMakeLists.txt to compile it
 * 4. Shows you how to build and run it
 */

#include "include/tinyllvm_compiler.h"
#include "include/eventchains.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void save_to_file(const char *filename, const char *content) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not open %s for writing\n", filename);
        return;
    }
    fprintf(f, "%s", content);
    fclose(f);
    printf("✓ Saved to: %s\n", filename);
}

int main(void) {
    printf("=== TinyLLVM Compile & Save Test ===\n\n");

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

    printf("Source Code:\n");
    printf("============\n%s\n", source);

    /* Compile it */
    CompilerConfig config = {
        .target = TARGET_C,
        .enable_optimization = false,
        .emit_comments = true,
        .pretty_print = true
    };

    EventChain *chain = event_chain_create(FAULT_TOLERANCE_STRICT);

    event_chain_add_event(chain,
        chainable_event_create(compiler_lexer_event, NULL, "Lexer"));
    event_chain_add_event(chain,
        chainable_event_create(compiler_parser_event, NULL, "Parser"));
    event_chain_add_event(chain,
        chainable_event_create(compiler_type_checker_event, NULL, "TypeChecker"));
    event_chain_add_event(chain,
        chainable_event_create(compiler_codegen_event, &config, "CodeGen"));

    EventContext *ctx = event_chain_get_context(chain);
    char *source_copy = malloc(strlen(source) + 1);
    strcpy(source_copy, source);
    event_context_set_with_cleanup(ctx, "source_code", source_copy, free);

    printf("\nCompiling...\n");

    ChainResult result;
    event_chain_execute(chain, &result);

    if (!result.success) {
        printf("❌ Compilation FAILED\n");
        if (result.failure_count > 0) {
            FailureInfo *failures = (FailureInfo *)result.failures;
            for (size_t i = 0; i < result.failure_count; i++) {
                printf("  %s: %s\n", failures[i].event_name, failures[i].error_message);
            }
        }
        chain_result_destroy(&result);
        event_chain_destroy(chain);
        event_chain_cleanup();
        return 1;
    }

    printf("✓ Compilation successful!\n\n");

    /* Get generated code */
    char *output_code;
    event_context_get(ctx, "output_code", (void**)&output_code);

    printf("Generated C Code:\n");
    printf("=================\n%s\n", output_code);

    /* Save to files */
    printf("\nSaving files...\n");
    save_to_file("factorial.c", output_code);

    /* Create a simple CMakeLists.txt for the generated program */
    const char *cmake_content =
        "cmake_minimum_required(VERSION 3.10)\n"
        "project(GeneratedProgram C)\n"
        "\n"
        "add_executable(factorial factorial.c)\n"
        "\n"
        "# For Windows: no special flags needed\n"
        "# For Linux: no special flags needed\n";

    save_to_file("factorial_CMakeLists.txt", cmake_content);

    printf("\n");
    printf("================================================================\n");
    printf("SUCCESS! Files saved.\n");
    printf("================================================================\n\n");

    printf("To compile and run the generated program:\n\n");

    printf("Method 1 - Using Visual Studio Developer Command Prompt:\n");
    printf("  cl factorial.c /Fe:factorial.exe\n");
    printf("  factorial.exe\n\n");

    printf("Method 2 - Using CMake (recommended):\n");
    printf("  mkdir factorial_build\n");
    printf("  cd factorial_build\n");
    printf("  cmake .. -G \"Visual Studio 17 2022\"\n");
    printf("  cmake --build .\n");
    printf("  Debug\\factorial.exe\n\n");

    printf("Method 3 - Using cl directly:\n");
    printf("  cl /nologo factorial.c\n");
    printf("  factorial.exe\n\n");

    printf("Expected output: 120\n");
    printf("(5! = 5 × 4 × 3 × 2 × 1 = 120)\n\n");

    chain_result_destroy(&result);
    event_chain_destroy(chain);
    event_chain_cleanup();

    return 0;
}