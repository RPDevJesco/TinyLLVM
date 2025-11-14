/* ==================== ADVERSARIAL MIDDLEWARE: Input Fuzzer ==================== */

#ifndef INPUT_FUZZER_MIDDLEWARE_H
#define INPUT_FUZZER_MIDDLEWARE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eventchains.h"

/* Simple cleanup function for strings */
static void cleanup_string_fuzzer(void *ptr) {
    free(ptr);
}

void input_fuzzer_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    (void)user_data;

    /* Only fuzz the lexer stage */
    if (strcmp(event->name, "Lexer") != 0) {
        next(result_ptr, event, context, next_data);
        return;
    }

    void *source_ptr = NULL;
    event_context_get(context, "source", &source_ptr);

    if (source_ptr) {
        const char *original = (const char *)source_ptr;
        printf("[InputFuzzer] Original input: \"%s\"\n", original);

        /* Randomly decide whether to fuzz */
        if (rand() % 5 == 0) {  /* 20% chance */
            /* Create fuzzed input */
            size_t len = strlen(original);
            char *fuzzed = malloc(len + 10);
            if (fuzzed) {
                strcpy(fuzzed, original);

                /* Add random garbage */
                if (len > 0 && rand() % 2 == 0) {
                    fuzzed[len] = '@';
                    fuzzed[len + 1] = '\0';
                    printf("[InputFuzzer] 🐛 Fuzzing input by adding garbage\n");

                    event_context_set_with_cleanup(context, "source", fuzzed,
                                                  cleanup_string_fuzzer);
                } else {
                    free(fuzzed);
                }
            }
        }
    }

    next(result_ptr, event, context, next_data);
}

#endif /* INPUT_FUZZER_MIDDLEWARE_H */