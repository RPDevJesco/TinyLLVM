/* ==================== ADVERSARIAL MIDDLEWARE: Integer Overflow Fuzzer ==================== */

#ifndef INTEGER_OVERFLOW_FUZZER_H
#define INTEGER_OVERFLOW_FUZZER_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "eventchains.h"

/**
 * Integer Overflow Fuzzer
 *
 * Injects edge-case integer values and validates arithmetic operations:
 * - INT_MAX, INT_MIN values
 * - Values near overflow boundaries
 * - Detects signed integer overflows
 * - Validates division by zero
 * - Checks for undefined behavior in arithmetic
 */

typedef struct {
    bool enabled;
    double injection_rate;    /* 0.0 to 1.0 - probability of injecting edge case */
    bool inject_max_values;   /* Inject INT_MAX */
    bool inject_min_values;   /* Inject INT_MIN */
    bool inject_near_zero;    /* Inject -1, 0, 1 */
    bool inject_large_primes; /* Inject large prime numbers */
    bool detect_overflows;    /* Monitor for overflow conditions */
    bool strict_mode;         /* Fail on overflow detection */

    /* Statistics */
    size_t injections_performed;
    size_t overflows_detected;
    size_t division_by_zero_detected;
} IntOverflowConfig;

/**
 * Edge case values for injection
 */
static const int EDGE_CASE_VALUES[] = {
    0,
    1,
    -1,
    INT_MAX,
    INT_MIN,
    INT_MAX - 1,
    INT_MIN + 1,
    INT_MAX / 2,
    INT_MIN / 2,
    32767,      /* 16-bit max */
    -32768,     /* 16-bit min */
    2147483647, /* Explicit INT_MAX */
    -2147483647 - 1, /* Explicit INT_MIN */
    999999999,
    -999999999
};

#define EDGE_CASE_COUNT (sizeof(EDGE_CASE_VALUES) / sizeof(EDGE_CASE_VALUES[0]))

/**
 * Check if addition would overflow
 */
static bool would_add_overflow(int a, int b) {
    if (b > 0 && a > INT_MAX - b) return true;
    if (b < 0 && a < INT_MIN - b) return true;
    return false;
}

/**
 * Check if subtraction would overflow
 */
static bool would_sub_overflow(int a, int b) {
    if (b < 0 && a > INT_MAX + b) return true;
    if (b > 0 && a < INT_MIN + b) return true;
    return false;
}

/**
 * Check if multiplication would overflow
 */
static bool would_mul_overflow(int a, int b) {
    if (a == 0 || b == 0) return false;

    /* Check for INT_MIN * -1 overflow */
    if (a == -1 && b == INT_MIN) return true;
    if (b == -1 && a == INT_MIN) return true;

    /* General overflow check */
    if (a > 0 && b > 0 && a > INT_MAX / b) return true;
    if (a > 0 && b < 0 && b < INT_MIN / a) return true;
    if (a < 0 && b > 0 && a < INT_MIN / b) return true;
    if (a < 0 && b < 0 && a < INT_MAX / b) return true;

    return false;
}

/**
 * Check if division would overflow
 */
static bool would_div_overflow(int a, int b) {
    if (b == 0) return true;  /* Division by zero */
    if (a == INT_MIN && b == -1) return true;  /* Special overflow case */
    return false;
}

/**
 * Inject edge-case values into source if applicable
 */
static void inject_edge_cases_into_source(
    IntOverflowConfig *config,
    EventContext *context,
    const char *event_name
) {
    if (!config->enabled) return;

    /* Only inject in lexer stage */
    if (strcmp(event_name, "Lexer") != 0) return;

    /* Random decision to inject */
    double roll = (double)rand() / RAND_MAX;
    if (roll >= config->injection_rate) return;

    void *source_ptr = NULL;
    if (event_context_get(context, "source", &source_ptr) != EC_SUCCESS || !source_ptr) {
        return;
    }

    const char *original = (const char *)source_ptr;

    /* Select a random edge case value */
    int edge_value = EDGE_CASE_VALUES[rand() % EDGE_CASE_COUNT];

    /* Create modified source with edge case */
    char *modified = malloc(512);
    if (!modified) return;

    /* Replace first number with edge case */
    if (config->inject_max_values && rand() % 2 == 0) {
        edge_value = INT_MAX;
    } else if (config->inject_min_values && rand() % 2 == 0) {
        edge_value = INT_MIN;
    } else if (config->inject_near_zero && rand() % 3 == 0) {
        int near_zero[] = {-1, 0, 1};
        edge_value = near_zero[rand() % 3];
    }

    /* Find first number in expression and replace it */
    const char *p = original;
    while (*p && !isdigit((unsigned char)*p) && *p != '-') p++;

    if (*p) {
        size_t prefix_len = p - original;
        snprintf(modified, 512, "%.*s%d", (int)prefix_len, original, edge_value);

        /* Skip the original number */
        if (*p == '-') p++;
        while (*p && isdigit((unsigned char)*p)) p++;

        /* Append rest of expression */
        strncat(modified, p, 512 - strlen(modified) - 1);

        printf("[IntOverflowFuzzer] 🎲 Injecting edge case: %d\n", edge_value);
        printf("  Original: \"%s\"\n", original);
        printf("  Modified: \"%s\"\n", modified);

        /* Set modified source with cleanup function */
        event_context_set_with_cleanup(context, "source", modified, free);
        config->injections_performed++;
    } else {
        free(modified);
    }
}

/**
 * Validate bytecode for potential overflows
 */
static bool validate_bytecode_for_overflow(
    IntOverflowConfig *config,
    EventContext *context,
    const char *event_name
) {
    if (!config->enabled || !config->detect_overflows) return true;

    /* Only check after optimizer */
    if (strcmp(event_name, "Optimizer") != 0 && strcmp(event_name, "CodeGen") != 0) {
        return true;
    }

    void *code_ptr = NULL;
    if (event_context_get(context, "bytecode", &code_ptr) != EC_SUCCESS || !code_ptr) {
        return true;
    }

    /* We need to import bytecode definition */
    typedef enum {
        INSTR_PUSH,
        INSTR_ADD,
        INSTR_SUB,
        INSTR_MUL,
        INSTR_DIV
    } InstructionType;

    typedef struct {
        InstructionType type;
        int operand;
    } Instruction;

    typedef struct {
        Instruction *instructions;
        size_t count;
        size_t capacity;
    } ByteCode;

    ByteCode *code = (ByteCode *)code_ptr;

    /* Simulate execution to check for overflows */
    int stack[256];
    int stack_ptr = 0;
    bool overflow_detected = false;

    printf("[IntOverflowFuzzer] 🔍 Analyzing %zu instructions for overflow\n", code->count);

    for (size_t i = 0; i < code->count; i++) {
        Instruction instr = code->instructions[i];

        switch (instr.type) {
            case INSTR_PUSH:
                if (stack_ptr >= 256) return true;  /* Stack overflow, but not our concern */
                stack[stack_ptr++] = instr.operand;

                /* Check for edge values */
                if (instr.operand == INT_MAX || instr.operand == INT_MIN) {
                    printf("[IntOverflowFuzzer] ⚠️  Edge value on stack: %d\n", instr.operand);
                }
                break;

            case INSTR_ADD:
                if (stack_ptr < 2) return true;
                {
                    int b = stack[--stack_ptr];
                    int a = stack[--stack_ptr];

                    if (would_add_overflow(a, b)) {
                        printf("[IntOverflowFuzzer] 🔥 OVERFLOW DETECTED: %d + %d\n", a, b);
                        overflow_detected = true;
                        config->overflows_detected++;
                    }

                    stack[stack_ptr++] = a + b;  /* Still compute for analysis */
                }
                break;

            case INSTR_SUB:
                if (stack_ptr < 2) return true;
                {
                    int b = stack[--stack_ptr];
                    int a = stack[--stack_ptr];

                    if (would_sub_overflow(a, b)) {
                        printf("[IntOverflowFuzzer] 🔥 OVERFLOW DETECTED: %d - %d\n", a, b);
                        overflow_detected = true;
                        config->overflows_detected++;
                    }

                    stack[stack_ptr++] = a - b;
                }
                break;

            case INSTR_MUL:
                if (stack_ptr < 2) return true;
                {
                    int b = stack[--stack_ptr];
                    int a = stack[--stack_ptr];

                    if (would_mul_overflow(a, b)) {
                        printf("[IntOverflowFuzzer] 🔥 OVERFLOW DETECTED: %d * %d\n", a, b);
                        overflow_detected = true;
                        config->overflows_detected++;
                    }

                    stack[stack_ptr++] = a * b;
                }
                break;

            case INSTR_DIV:
                if (stack_ptr < 2) return true;
                {
                    int b = stack[--stack_ptr];
                    int a = stack[--stack_ptr];

                    if (b == 0) {
                        printf("[IntOverflowFuzzer] 🔥 DIVISION BY ZERO: %d / 0\n", a);
                        config->division_by_zero_detected++;
                        overflow_detected = true;
                    } else if (would_div_overflow(a, b)) {
                        printf("[IntOverflowFuzzer] 🔥 OVERFLOW DETECTED: %d / %d (INT_MIN / -1)\n", a, b);
                        overflow_detected = true;
                        config->overflows_detected++;
                    }

                    if (b != 0) {
                        stack[stack_ptr++] = a / b;
                    } else {
                        stack[stack_ptr++] = 0;  /* Dummy value */
                    }
                }
                break;
        }
    }

    if (overflow_detected && config->strict_mode) {
        return false;
    }

    return true;
}

/**
 * Integer overflow fuzzer middleware
 */
void integer_overflow_fuzzer_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    IntOverflowConfig *config = (IntOverflowConfig *)user_data;

    if (!config || !config->enabled) {
        next(result_ptr, event, context, next_data);
        return;
    }

    printf("[IntOverflowFuzzer] === Fuzzing %s ===\n", event->name);

    /* Inject edge cases before execution */
    inject_edge_cases_into_source(config, context, event->name);

    /* Execute the event */
    next(result_ptr, event, context, next_data);

    /* Validate for overflows after execution */
    bool valid = validate_bytecode_for_overflow(config, context, event->name);

    if (!valid && config->strict_mode) {
        printf("[IntOverflowFuzzer] ❌ Integer overflow detected (strict mode)\n");
        event_result_failure(
            result_ptr,
            "Integer overflow detected during execution",
            EC_ERROR_OVERFLOW,
            ERROR_DETAIL_FULL
        );
    }
}

/**
 * Create integer overflow fuzzer configuration
 */
static IntOverflowConfig *int_overflow_fuzzer_create(bool strict_mode) {
    IntOverflowConfig *config = calloc(1, sizeof(IntOverflowConfig));
    if (!config) return NULL;

    config->enabled = true;
    config->injection_rate = 0.3;  /* 30% chance */
    config->inject_max_values = true;
    config->inject_min_values = true;
    config->inject_near_zero = true;
    config->inject_large_primes = false;
    config->detect_overflows = true;
    config->strict_mode = strict_mode;
    config->injections_performed = 0;
    config->overflows_detected = 0;
    config->division_by_zero_detected = 0;

    return config;
}

/**
 * Print fuzzer summary
 */
static void int_overflow_fuzzer_print_summary(IntOverflowConfig *config) {
    if (!config) return;

    printf("\n=== Integer Overflow Fuzzer Summary ===\n");
    printf("Edge case injections: %zu\n", config->injections_performed);
    printf("Overflows detected: %zu\n", config->overflows_detected);
    printf("Division by zero: %zu\n", config->division_by_zero_detected);
    printf("=======================================\n\n");
}

#endif /* INTEGER_OVERFLOW_FUZZER_H */