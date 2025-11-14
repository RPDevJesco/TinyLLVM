/* ==================== ADVERSARIAL MIDDLEWARE: Context Corruptor ==================== */

#ifndef CONTEXT_CORRUPTOR_MIDDLEWARE_H
#define CONTEXT_CORRUPTOR_MIDDLEWARE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eventchains.h"

/* Simple cleanup function for strings */
static void cleanup_string(void *ptr) {
    free(ptr);
}

void context_corruptor_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    (void)user_data;

    /* Execute the event first */
    next(result_ptr, event, context, next_data);

    /* Randomly corrupt context after event execution */
    if (result_ptr->success && rand() % 10 == 0) {  /* 10% chance */
        printf("[ContextCorruptor] 👹 Corrupting context after %s\n",
               event->name);

        /* Add bogus entry */
        char *bogus = strdup("corrupted_data");
        if (bogus) {
            event_context_set_with_cleanup(context, "!!!CORRUPTED!!!", bogus,
                                          cleanup_string);
        }
    }
}

#endif /* CONTEXT_CORRUPTOR_MIDDLEWARE_H */