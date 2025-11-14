/* ==================== MIDDLEWARE: Timing ==================== */

#ifndef TIMING_MIDDLEWARE_H
#define TIMING_MIDDLEWARE_H

#include <stdio.h>
#include <time.h>
#include "eventchains.h"

void timing_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    (void)user_data;

    clock_t start = clock();

    /* Execute the wrapped event */
    next(result_ptr, event, context, next_data);

    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("[Timing] %s took %.3f ms\n", event->name, elapsed);
}

#endif /* TIMING_MIDDLEWARE_H */