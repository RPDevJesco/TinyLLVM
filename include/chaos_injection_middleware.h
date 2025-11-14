/* ==================== ADVERSARIAL MIDDLEWARE: Chaos Injection ==================== */

#ifndef CHAOS_INJECTION_MIDDLEWARE_H
#define CHAOS_INJECTION_MIDDLEWARE_H

#include <stdio.h>
#include <stdlib.h>
#include "eventchains.h"

typedef struct {
    double failure_rate;  /* 0.0 to 1.0 */
    bool enabled;
} ChaosConfig;

void chaos_injection_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
) {
    ChaosConfig *config = (ChaosConfig *)user_data;

    if (!config || !config->enabled) {
        next(result_ptr, event, context, next_data);
        return;
    }

    /* Randomly inject failures */
    double roll = (double)rand() / RAND_MAX;

    if (roll < config->failure_rate) {
        printf("[ChaosInjection] 💥 Injecting random failure in %s!\n",
               event->name);
        event_result_failure(
            result_ptr,
            "Chaos monkey struck!",
            EC_ERROR_INVALID_PARAMETER,
            ERROR_DETAIL_FULL
        );
        return;
    }

    next(result_ptr, event, context, next_data);
}

#endif /* CHAOS_INJECTION_MIDDLEWARE_H */