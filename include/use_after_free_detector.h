/* ==================== ADVERSARIAL MIDDLEWARE: Use-After-Free Detector ==================== */

#ifndef USE_AFTER_FREE_DETECTOR_H
#define USE_AFTER_FREE_DETECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "eventchains.h"

/**
 * Use-After-Free Detector
 *
 * Tracks memory allocations and accesses through the context to detect:
 * - Access to memory after it has been freed
 * - Double-free attempts
 * - Invalid pointer dereferences
 *
 * This middleware maintains a shadow registry of all context values and
 * validates that accessed memory is still valid.
 */

#define MAX_TRACKED_ALLOCATIONS 1024
#define UAF_POISON_VALUE 0xDEADBEEF

typedef enum {
    ALLOC_STATE_ACTIVE,
    ALLOC_STATE_FREED,
    ALLOC_STATE_INVALID
} AllocationState;

typedef struct {
    void *ptr;
    size_t size;
    AllocationState state;
    char key[256];
    char event_name[256];
    bool is_tracked;
} TrackedAllocation;

typedef struct {
    TrackedAllocation allocations[MAX_TRACKED_ALLOCATIONS];
    size_t count;
    bool enabled;
    bool poison_freed_memory;  /* Fill freed memory with poison pattern */
    bool strict_mode;          /* Fail on any UAF detection */
    size_t uaf_detected_count;
    size_t double_free_count;
} UAFDetectorConfig;

/**
 * Find a tracked allocation by pointer
 */
static TrackedAllocation *find_allocation(UAFDetectorConfig *config, void *ptr) {
    if (!config || !ptr) return NULL;

    for (size_t i = 0; i < config->count; i++) {
        if (config->allocations[i].ptr == ptr) {
            return &config->allocations[i];
        }
    }

    return NULL;
}

/**
 * Track a new allocation
 */
static bool track_allocation(
    UAFDetectorConfig *config,
    void *ptr,
    size_t size,
    const char *key,
    const char *event_name
) {
    if (!config || !ptr) return false;

    if (config->count >= MAX_TRACKED_ALLOCATIONS) {
        printf("[UAFDetector] ⚠️  Warning: Allocation tracking limit reached\n");
        return false;
    }

    /* Check if already tracked (shouldn't happen) */
    TrackedAllocation *existing = find_allocation(config, ptr);
    if (existing) {
        printf("[UAFDetector] ⚠️  Warning: Pointer %p already tracked\n", ptr);
        return false;
    }

    /* Add new tracking entry */
    TrackedAllocation *alloc = &config->allocations[config->count++];
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->state = ALLOC_STATE_ACTIVE;
    alloc->is_tracked = true;

    strncpy(alloc->key, key ? key : "", sizeof(alloc->key) - 1);
    alloc->key[sizeof(alloc->key) - 1] = '\0';

    strncpy(alloc->event_name, event_name ? event_name : "", sizeof(alloc->event_name) - 1);
    alloc->event_name[sizeof(alloc->event_name) - 1] = '\0';

    printf("[UAFDetector] 📍 Tracking allocation: %p (%zu bytes) for key '%s' in %s\n",
           ptr, size, key, event_name);

    return true;
}

/**
 * Mark an allocation as freed
 */
static bool mark_freed(UAFDetectorConfig *config, void *ptr) {
    if (!config || !ptr) return false;

    TrackedAllocation *alloc = find_allocation(config, ptr);
    if (!alloc) {
        printf("[UAFDetector] ⚠️  Warning: Attempted to free untracked pointer %p\n", ptr);
        return false;
    }

    if (alloc->state == ALLOC_STATE_FREED) {
        printf("[UAFDetector] 🔥 DOUBLE-FREE DETECTED: %p (key '%s')\n",
               ptr, alloc->key);
        config->double_free_count++;
        return false;
    }

    alloc->state = ALLOC_STATE_FREED;

    /* Poison the memory if enabled */
    if (config->poison_freed_memory && alloc->size > 0) {
        memset(ptr, (UAF_POISON_VALUE & 0xFF), alloc->size);
        printf("[UAFDetector] 💀 Poisoned freed memory: %p (%zu bytes)\n",
               ptr, alloc->size);
    }

    printf("[UAFDetector] ✓ Marked as freed: %p (key '%s')\n", ptr, alloc->key);

    return true;
}

/**
 * Check if pointer access is valid
 */
static bool validate_access(UAFDetectorConfig *config, void *ptr, const char *key) {
    if (!config || !ptr) return true;  /* NULL is technically valid */

    TrackedAllocation *alloc = find_allocation(config, ptr);

    if (!alloc) {
        /* Not tracked - might be OK if it's external memory */
        return true;
    }

    if (alloc->state == ALLOC_STATE_FREED) {
        printf("[UAFDetector] 🔥 USE-AFTER-FREE DETECTED!\n");
        printf("  Pointer: %p\n", ptr);
        printf("  Key: '%s'\n", key);
        printf("  Original allocation: '%s' in event '%s'\n",
               alloc->key, alloc->event_name);
        printf("  Memory was freed but is being accessed\n");

        config->uaf_detected_count++;
        return false;
    }

    if (alloc->state == ALLOC_STATE_INVALID) {
        printf("[UAFDetector] 🔥 INVALID MEMORY ACCESS: %p (key '%s')\n",
               ptr, key);
        return false;
    }

    return true;
}

/**
 * Scan context for any UAF violations
 */
static bool scan_context_for_uaf(
    UAFDetectorConfig *config,
    EventContext *context,
    const char *event_name
) {
    if (!config || !context) return true;

    bool all_valid = true;
    size_t context_count = event_context_count(context);

    printf("[UAFDetector] 🔍 Scanning context (%zu entries) in %s\n",
           context_count, event_name);

    /* We can't directly iterate context entries without exposing internals,
     * so we'll check known keys that might contain pointers */
    const char *known_keys[] = {
        "tokens", "ast", "bytecode", "result",
        "source", "constant_value", NULL
    };

    for (int i = 0; known_keys[i] != NULL; i++) {
        void *ptr = NULL;
        EventChainErrorCode result = event_context_get(context, known_keys[i], &ptr);

        if (result == EC_SUCCESS && ptr != NULL) {
            if (!validate_access(config, ptr, known_keys[i])) {
                all_valid = false;

                if (config->strict_mode) {
                    break;  /* Stop on first violation in strict mode */
                }
            }
        }
    }

    return all_valid;
}

/**
 * Use-After-Free detector middleware
 */
void use_after_free_detector_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    UAFDetectorConfig *config = (UAFDetectorConfig *)user_data;

    if (!config || !config->enabled) {
        next(result_ptr, event, context, next_data);
        return;
    }

    printf("[UAFDetector] === Checking %s (BEFORE) ===\n", event->name);

    /* Scan context before event execution */
    bool valid_before = scan_context_for_uaf(config, context, event->name);

    if (!valid_before && config->strict_mode) {
        printf("[UAFDetector] ❌ UAF detected before event execution (strict mode)\n");
        event_result_failure(
            result_ptr,
            "Use-after-free detected before event execution",
            EC_ERROR_INVALID_PARAMETER,
            ERROR_DETAIL_FULL
        );
        return;
    }

    /* Execute the event */
    next(result_ptr, event, context, next_data);

    printf("[UAFDetector] === Checking %s (AFTER) ===\n", event->name);

    /* Scan context after event execution */
    bool valid_after = scan_context_for_uaf(config, context, event->name);

    if (!valid_after && config->strict_mode) {
        printf("[UAFDetector] ❌ UAF detected after event execution (strict mode)\n");

        /* Override success with failure in strict mode */
        event_result_failure(
            result_ptr,
            "Use-after-free detected after event execution",
            EC_ERROR_INVALID_PARAMETER,
            ERROR_DETAIL_FULL
        );
    }
}

/**
 * Create and initialize UAF detector configuration
 */
static UAFDetectorConfig *uaf_detector_create(bool strict_mode, bool poison_memory) {
    UAFDetectorConfig *config = calloc(1, sizeof(UAFDetectorConfig));
    if (!config) return NULL;

    config->enabled = true;
    config->poison_freed_memory = poison_memory;
    config->strict_mode = strict_mode;
    config->count = 0;
    config->uaf_detected_count = 0;
    config->double_free_count = 0;

    return config;
}

/**
 * Print detection summary
 */
static void uaf_detector_print_summary(UAFDetectorConfig *config) {
    if (!config) return;

    printf("\n=== Use-After-Free Detector Summary ===\n");
    printf("Tracked allocations: %zu\n", config->count);
    printf("UAF violations detected: %zu\n", config->uaf_detected_count);
    printf("Double-free attempts: %zu\n", config->double_free_count);

    size_t active = 0, freed = 0;
    for (size_t i = 0; i < config->count; i++) {
        if (config->allocations[i].state == ALLOC_STATE_ACTIVE) active++;
        else if (config->allocations[i].state == ALLOC_STATE_FREED) freed++;
    }

    printf("Current state: %zu active, %zu freed\n", active, freed);
    printf("======================================\n\n");
}

#endif /* USE_AFTER_FREE_DETECTOR_H */