#include "include/tinyllvm_compiler.h"
#include "include/eventchains.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("=== TinyLLVM IR Generation Test ===\n\n");

    event_chain_initialize();

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

    /* Configure for IR output */
    CompilerConfig config = {
        .target = TARGET_TINYLLVM,      // ← IR mode!
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
    char *source_copy = strdup(source);
    event_context_set_with_cleanup(ctx, "source_code", source_copy, free);

    printf("Compiling to TinyLLVM IR...\n\n");

    ChainResult result;
    event_chain_execute(chain, &result);

    if (result.success) {
        char *output;
        event_context_get(ctx, "output_code", (void**)&output);

        printf("Generated TinyLLVM IR:\n");
        printf("=====================\n");
        printf("%s\n", output);

        /* Optionally save to file */
        FILE *f = fopen("factorial.ll", "w");
        if (f) {
            fprintf(f, "%s", output);
            fclose(f);
            printf("\n✓ Saved to: factorial.ll\n");
        }
    } else {
        printf(" Compilation FAILED\n");
    }

    chain_result_destroy(&result);
    event_chain_destroy(chain);
    event_chain_cleanup();

    return result.success ? 0 : 1;
}