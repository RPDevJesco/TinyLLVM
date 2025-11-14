/* ==================== ADVERSARIAL MIDDLEWARE: Resource Exhaustion ==================== */

#ifndef RESOURCE_LIMIT_MIDDLEWARE_H
#define RESOURCE_LIMIT_MIDDLEWARE_H

#include <stdio.h>
#include "eventchains.h"

typedef struct {
    size_t max_memory;
    bool enabled;
} ResourceConfig;

void resource_limit_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    ResourceConfig *config = (ResourceConfig *)user_data;

    if (!config || !config->enabled) {
        next(result_ptr, event, context, next_data);
        return;
    }

    size_t current_memory = event_context_memory_usage(context);

    if (current_memory > config->max_memory) {
        printf("[ResourceLimit] ⚠️  Memory limit exceeded in %s: %zu > %zu bytes\n",
               event->name, current_memory, config->max_memory);
        event_result_failure(
            result_ptr,
            "Memory limit exceeded",
            EC_ERROR_MEMORY_LIMIT_EXCEEDED,
            ERROR_DETAIL_FULL
        );
        return;
    }

    /* Execute the event */
    next(result_ptr, event, context, next_data);

    current_memory = event_context_memory_usage(context);
    if (current_memory > config->max_memory) {
        printf("[ResourceLimit] ⚠️  Memory limit exceeded after %s: %zu > %zu bytes\n",
               event->name, current_memory, config->max_memory);
    }
}

#endif /* RESOURCE_LIMIT_MIDDLEWARE_H */