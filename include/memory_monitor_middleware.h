/* ==================== MIDDLEWARE: Memory Monitor ==================== */

#ifndef MEMORY_MONITOR_MIDDLEWARE_H
#define MEMORY_MONITOR_MIDDLEWARE_H

#include <stdio.h>
#include "eventchains.h"

void memory_monitor_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    (void)user_data;

    size_t before = event_context_memory_usage(context);

    /* Execute the wrapped event */
    next(result_ptr, event, context, next_data);

    size_t after = event_context_memory_usage(context);
    long delta = (long)after - (long)before;

    printf("[MemoryMonitor] %s: %+ld bytes (total: %zu bytes)\n",
           event->name, delta, after);
}

#endif /* MEMORY_MONITOR_MIDDLEWARE_H */