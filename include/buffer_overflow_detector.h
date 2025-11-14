/* ==================== ADVERSARIAL MIDDLEWARE: Buffer Overflow Detector ==================== */

#ifndef BUFFER_OVERFLOW_DETECTOR_H
#define BUFFER_OVERFLOW_DETECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "eventchains.h"

/**
 * Buffer Overflow Detector
 *
 * Monitors buffer operations and detects:
 * - Stack buffer overflows
 * - Heap buffer overflows
 * - String buffer overruns
 * - Array out-of-bounds access
 * - Unsafe string operations
 *
 * Uses canary values and bounds checking to detect overflows.
 */

#define CANARY_VALUE 0xDEADC0DE
#define MAX_TRACKED_BUFFERS 512
#define GUARD_BAND_SIZE 16

typedef struct {
    void *buffer;
    size_t size;
    uint32_t pre_canary;
    uint32_t post_canary;
    char name[256];
    char event_name[256];
    bool is_active;
} TrackedBuffer;

typedef struct {
    TrackedBuffer buffers[MAX_TRACKED_BUFFERS];
    size_t count;
    bool enabled;
    bool use_guard_bands;    /* Add guard bands around allocations */
    bool check_on_access;    /* Validate on every access */
    bool strict_mode;        /* Fail immediately on overflow */

    /* Statistics */
    size_t buffers_tracked;
    size_t overflows_detected;
    size_t underflows_detected;
    size_t oob_access_detected;
} BufferOverflowConfig;

/**
 * Initialize canaries for a buffer
 */
static void init_canaries(TrackedBuffer *buf) {
    if (!buf || !buf->buffer) return;

    buf->pre_canary = CANARY_VALUE;
    buf->post_canary = CANARY_VALUE;

    /* Write canaries to memory if using guard bands */
    if (buf->size >= 8) {
        uint32_t *pre = (uint32_t *)buf->buffer;
        uint32_t *post = (uint32_t *)((char *)buf->buffer + buf->size - 4);
        *pre = CANARY_VALUE;
        *post = CANARY_VALUE;
    }
}

/**
 * Check if canaries are intact
 */
static bool check_canaries(TrackedBuffer *buf, BufferOverflowConfig *config) {
    if (!buf || !buf->is_active) return true;

    bool intact = true;

    /* Check stored canaries */
    if (buf->pre_canary != CANARY_VALUE) {
        printf("[BufferOverflow] 🔥 PRE-CANARY CORRUPTED for buffer '%s'\n", buf->name);
        printf("  Expected: 0x%08X, Found: 0x%08X\n", CANARY_VALUE, buf->pre_canary);
        config->underflows_detected++;
        intact = false;
    }

    if (buf->post_canary != CANARY_VALUE) {
        printf("[BufferOverflow] 🔥 POST-CANARY CORRUPTED for buffer '%s'\n", buf->name);
        printf("  Expected: 0x%08X, Found: 0x%08X\n", CANARY_VALUE, buf->post_canary);
        config->overflows_detected++;
        intact = false;
    }

    /* Check memory canaries if using guard bands */
    if (config->use_guard_bands && buf->buffer && buf->size >= 8) {
        uint32_t *pre = (uint32_t *)buf->buffer;
        uint32_t *post = (uint32_t *)((char *)buf->buffer + buf->size - 4);

        if (*pre != CANARY_VALUE) {
            printf("[BufferOverflow] 🔥 MEMORY UNDERFLOW for buffer '%s'\n", buf->name);
            printf("  Pre-guard corrupted: Expected 0x%08X, Found 0x%08X\n",
                   CANARY_VALUE, *pre);
            config->underflows_detected++;
            intact = false;
        }

        if (*post != CANARY_VALUE) {
            printf("[BufferOverflow] 🔥 MEMORY OVERFLOW for buffer '%s'\n", buf->name);
            printf("  Post-guard corrupted: Expected 0x%08X, Found 0x%08X\n",
                   CANARY_VALUE, *post);
            config->overflows_detected++;
            intact = false;
        }
    }

    return intact;
}

/**
 * Track a new buffer
 */
static bool track_buffer(
    BufferOverflowConfig *config,
    void *buffer,
    size_t size,
    const char *name,
    const char *event_name
) {
    if (!config || !buffer || size == 0) return false;

    if (config->count >= MAX_TRACKED_BUFFERS) {
        printf("[BufferOverflow] ⚠️  Warning: Buffer tracking limit reached\n");
        return false;
    }

    TrackedBuffer *buf = &config->buffers[config->count++];
    buf->buffer = buffer;
    buf->size = size;
    buf->is_active = true;

    strncpy(buf->name, name ? name : "", sizeof(buf->name) - 1);
    buf->name[sizeof(buf->name) - 1] = '\0';

    strncpy(buf->event_name, event_name ? event_name : "", sizeof(buf->event_name) - 1);
    buf->event_name[sizeof(buf->event_name) - 1] = '\0';

    init_canaries(buf);

    printf("[BufferOverflow] 📍 Tracking buffer: %p (%zu bytes) '%s' in %s\n",
           buffer, size, name, event_name);

    config->buffers_tracked++;

    return true;
}

/**
 * Find tracked buffer by pointer
 */
static TrackedBuffer *find_buffer(BufferOverflowConfig *config, void *ptr) {
    if (!config || !ptr) return NULL;

    for (size_t i = 0; i < config->count; i++) {
        TrackedBuffer *buf = &config->buffers[i];
        if (buf->is_active && buf->buffer == ptr) {
            return buf;
        }

        /* Check if pointer is within buffer range */
        if (buf->is_active &&
            ptr >= buf->buffer &&
            ptr < (void *)((char *)buf->buffer + buf->size)) {
            return buf;
        }
    }

    return NULL;
}

/**
 * Check string operation safety
 */
static bool check_string_operation(
    BufferOverflowConfig *config,
    const char *str,
    size_t max_len,
    const char *operation
) {
    if (!config || !str) return true;

    TrackedBuffer *buf = find_buffer(config, (void *)str);
    if (!buf) {
        /* Not a tracked buffer */
        return true;
    }

    /* Check if string fits in buffer */
    size_t str_len = strnlen(str, max_len);

    if (str_len >= buf->size) {
        printf("[BufferOverflow] 🔥 STRING OVERFLOW in %s\n", operation);
        printf("  Buffer: '%s' (%p, %zu bytes)\n", buf->name, buf->buffer, buf->size);
        printf("  String length: %zu (exceeds buffer size)\n", str_len);
        config->overflows_detected++;
        return false;
    }

    return true;
}

/**
 * Validate all tracked buffers
 */
static bool validate_all_buffers(
    BufferOverflowConfig *config,
    const char *event_name
) {
    if (!config) return true;

    bool all_valid = true;
    size_t checked = 0;

    printf("[BufferOverflow] 🔍 Validating %zu buffers in %s\n",
           config->count, event_name);

    for (size_t i = 0; i < config->count; i++) {
        TrackedBuffer *buf = &config->buffers[i];
        if (!buf->is_active) continue;

        checked++;
        if (!check_canaries(buf, config)) {
            all_valid = false;

            if (config->strict_mode) {
                break;  /* Stop on first violation */
            }
        }
    }

    printf("[BufferOverflow] Checked %zu active buffers\n", checked);

    return all_valid;
}

/**
 * Check for suspicious buffer patterns
 */
static void check_context_buffers(
    BufferOverflowConfig *config,
    EventContext *context,
    const char *event_name
) {
    if (!config) return;

    /* Check known string buffers */
    void *source_ptr = NULL;
    if (event_context_get(context, "source", &source_ptr) == EC_SUCCESS && source_ptr) {
        const char *source = (const char *)source_ptr;

        /* Check for extremely long input (potential overflow attempt) */
        size_t len = strnlen(source, 10000);
        if (len > 1000) {
            printf("[BufferOverflow] ⚠️  Warning: Very long input string (%zu chars)\n", len);
        }

        /* Track this buffer */
        TrackedBuffer *existing = find_buffer(config, source_ptr);
        if (!existing) {
            track_buffer(config, source_ptr, len + 1, "source", event_name);
        }
    }

    /* Check token buffer */
    void *tokens_ptr = NULL;
    if (event_context_get(context, "tokens", &tokens_ptr) == EC_SUCCESS && tokens_ptr) {
        typedef struct {
            void *tokens;
            size_t count;
            size_t capacity;
        } TokenList;

        TokenList *token_list = (TokenList *)tokens_ptr;

        /* Check for buffer over-allocation */
        if (token_list->count > token_list->capacity) {
            printf("[BufferOverflow] 🔥 TOKEN BUFFER OVERFLOW!\n");
            printf("  Count: %zu, Capacity: %zu\n",
                   token_list->count, token_list->capacity);
            config->oob_access_detected++;
        }

        /* Track token buffer */
        if (token_list->tokens) {
            TrackedBuffer *existing = find_buffer(config, token_list->tokens);
            if (!existing) {
                size_t buffer_size = token_list->capacity * 32; /* Approx token size */
                track_buffer(config, token_list->tokens, buffer_size,
                           "tokens", event_name);
            }
        }
    }

    /* Check bytecode buffer */
    void *code_ptr = NULL;
    if (event_context_get(context, "bytecode", &code_ptr) == EC_SUCCESS && code_ptr) {
        typedef struct {
            void *instructions;
            size_t count;
            size_t capacity;
        } ByteCode;

        ByteCode *code = (ByteCode *)code_ptr;

        /* Check for buffer over-allocation */
        if (code->count > code->capacity) {
            printf("[BufferOverflow] 🔥 BYTECODE BUFFER OVERFLOW!\n");
            printf("  Count: %zu, Capacity: %zu\n", code->count, code->capacity);
            config->oob_access_detected++;
        }

        /* Track bytecode buffer */
        if (code->instructions) {
            TrackedBuffer *existing = find_buffer(config, code->instructions);
            if (!existing) {
                size_t buffer_size = code->capacity * 12; /* Approx instruction size */
                track_buffer(config, code->instructions, buffer_size,
                           "bytecode", event_name);
            }
        }
    }
}

/**
 * Buffer overflow detector middleware
 */
void buffer_overflow_detector_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    BufferOverflowConfig *config = (BufferOverflowConfig *)user_data;

    if (!config || !config->enabled) {
        next(result_ptr, event, context, next_data);
        return;
    }

    printf("[BufferOverflow] === Checking %s (BEFORE) ===\n", event->name);

    /* Check existing buffers */
    bool valid_before = validate_all_buffers(config, event->name);

    if (!valid_before && config->strict_mode) {
        printf("[BufferOverflow] ❌ Buffer overflow detected before event\n");
        event_result_failure(
            result_ptr,
            "Buffer overflow detected before event execution",
            EC_ERROR_INVALID_PARAMETER,
            ERROR_DETAIL_FULL
        );
        return;
    }

    /* Execute the event */
    next(result_ptr, event, context, next_data);

    printf("[BufferOverflow] === Checking %s (AFTER) ===\n", event->name);

    /* Track new buffers from context */
    check_context_buffers(config, context, event->name);

    /* Validate all buffers again */
    bool valid_after = validate_all_buffers(config, event->name);

    if (!valid_after && config->strict_mode) {
        printf("[BufferOverflow] ❌ Buffer overflow detected after event\n");
        event_result_failure(
            result_ptr,
            "Buffer overflow detected after event execution",
            EC_ERROR_INVALID_PARAMETER,
            ERROR_DETAIL_FULL
        );
    }
}

/**
 * Create buffer overflow detector configuration
 */
static BufferOverflowConfig *buffer_overflow_detector_create(
    bool strict_mode,
    bool use_guard_bands
) {
    BufferOverflowConfig *config = calloc(1, sizeof(BufferOverflowConfig));
    if (!config) return NULL;

    config->enabled = true;
    config->use_guard_bands = use_guard_bands;
    config->check_on_access = true;
    config->strict_mode = strict_mode;
    config->count = 0;
    config->buffers_tracked = 0;
    config->overflows_detected = 0;
    config->underflows_detected = 0;
    config->oob_access_detected = 0;

    return config;
}

/**
 * Print detection summary
 */
static void buffer_overflow_detector_print_summary(BufferOverflowConfig *config) {
    if (!config) return;

    printf("\n=== Buffer Overflow Detector Summary ===\n");
    printf("Buffers tracked: %zu\n", config->buffers_tracked);
    printf("Overflows detected: %zu\n", config->overflows_detected);
    printf("Underflows detected: %zu\n", config->underflows_detected);
    printf("Out-of-bounds access: %zu\n", config->oob_access_detected);
    printf("========================================\n\n");
}

#endif /* BUFFER_OVERFLOW_DETECTOR_H */