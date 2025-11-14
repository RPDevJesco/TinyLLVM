/* ==================== MIDDLEWARE: Logging ==================== */

#ifndef LOGGING_MIDDLEWARE_H
#define LOGGING_MIDDLEWARE_H

#include <stdio.h>
#include "eventchains.h"

void logging_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    (void)user_data;

    printf("[Logging] === Entering: %s ===\n", event->name);
    printf("[Logging] Context entries: %zu\n", event_context_count(context));

    /* Call next middleware/event */
    next(result_ptr, event, context, next_data);

    if (result_ptr->success) {
        printf("[Logging] === Completed: %s (SUCCESS) ===\n", event->name);
    } else {
        printf("[Logging] === Completed: %s (FAILED: %s) ===\n",
               event->name, result_ptr->error_message);
    }
}

#endif /* LOGGING_MIDDLEWARE_H */