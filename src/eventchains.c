/**
 * ==============================================================================
 * EventChains - High-Performance Event Processing Library
 * C Implementation
 * ==============================================================================
 * Version: 3.1.0
 *
 * This is a pure C implementation of the EventChains library, providing the
 * same API as the x86_64 assembly version.
 *
 * Copyright (c) 2024 EventChains Project
 * Licensed under the MIT License
 * ==============================================================================
 */

#include "include/eventchains.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ==============================================================================
 * Internal Constants
 * ==============================================================================
 */

#define INITIAL_CAPACITY 8
#define GROWTH_FACTOR 2
#define MIN_FUNCTION_POINTER 0x1000

/* ==============================================================================
 * Static Data
 * ==============================================================================
 */

static ec_atomic_uint64_t perf_stats[8] = {0};

static const char *error_strings[] = {
    "Success",
    "NULL pointer",
    "Invalid parameter",
    "Out of memory",
    "Capacity exceeded",
    "Key too long",
    "Name too long",
    "Not found",
    "Arithmetic overflow",
    "Event execution failed",
    "Middleware failed",
    "Reentrancy detected",
    "Memory limit exceeded",
    "Invalid function pointer",
    "Time conversion error",
    "Signal interrupted"
};

/* ==============================================================================
 * Core Module Implementation
 * ==============================================================================
 */

const char *event_chain_version_string(void) {
    return EVENTCHAINS_VERSION_STRING;
}

void event_chain_version_numbers(int *major, int *minor, int *patch) {
    if (major) *major = EVENTCHAINS_VERSION_MAJOR;
    if (minor) *minor = EVENTCHAINS_VERSION_MINOR;
    if (patch) *patch = EVENTCHAINS_VERSION_PATCH;
}

const char *event_chain_build_info(void) {
    return "EventChains v" EVENTCHAINS_VERSION_STRING "\n"
           "C Implementation\n"
           "Architecture: x86_64\n"
           "Features: Reference Counting, Memory Limits, Thread Safety\n"
           "Build: " __DATE__ " " __TIME__;
}

const char *event_chain_architecture_info(void) {
    return "Architecture: x86_64 (AMD64)\n"
           "Pointer Size: 64-bit\n"
           "Endianness: Little Endian";
}

const char *event_chain_features(void) {
    return "- Reference Counting\n"
           "- Memory Limits\n"
           "- Thread Safety\n"
           "- Middleware Pipeline\n"
           "- Fault Tolerance Modes\n"
           "- Context Management\n"
           "- Error Detail Levels";
}

const char *event_chain_copyright(void) {
    return "Copyright (c) 2024 EventChains Project\n"
           "Licensed under the MIT License";
}

const char *event_chain_error_string(EventChainErrorCode code) {
    if (code >= 0 && code <= EC_ERROR_SIGNAL_INTERRUPTED) {
        return error_strings[code];
    }
    return "Unknown error";
}

const uint64_t *event_chain_get_perf_stats(void) {
    return (const uint64_t *)perf_stats;
}

void event_chain_reset_perf_stats(void) {
    for (int i = 0; i < 8; i++) {
        ec_atomic_store(&perf_stats[i], 0);
    }
}

size_t event_chain_get_max_events(void) {
    return EVENTCHAINS_MAX_EVENTS;
}

size_t event_chain_get_max_middleware(void) {
    return EVENTCHAINS_MAX_MIDDLEWARE;
}

size_t event_chain_get_max_context_entries(void) {
    return EVENTCHAINS_MAX_CONTEXT_ENTRIES;
}

size_t event_chain_get_max_context_memory(void) {
    return EVENTCHAINS_MAX_CONTEXT_MEMORY;
}

void event_chain_initialize(void) {
    event_chain_reset_perf_stats();
}

void event_chain_cleanup(void) {
    /* No global cleanup needed in C version */
}

/* ==============================================================================
 * Error Module Implementation
 * ==============================================================================
 */

void event_result_success(EventResult *result) {
    if (!result) return;

    result->success = true;
    result->error_code = EC_SUCCESS;
    result->error_message[0] = '\0';
}

void event_result_failure(
    EventResult *result,
    const char *error_message,
    EventChainErrorCode error_code,
    ErrorDetailLevel detail_level
) {
    if (!result) return;

    result->success = false;
    result->error_code = error_code;

    if (detail_level == ERROR_DETAIL_MINIMAL) {
        snprintf(result->error_message, EVENTCHAINS_MAX_ERROR_LENGTH,
                 "Error code: %d", error_code);
    } else if (error_message) {
        safe_strncpy(result->error_message, error_message,
                    EVENTCHAINS_MAX_ERROR_LENGTH);
    } else {
        safe_strncpy(result->error_message, event_chain_error_string(error_code),
                    EVENTCHAINS_MAX_ERROR_LENGTH);
    }
}

EventResult *event_result_create_success(void) {
    EventResult *result = malloc(sizeof(EventResult));
    if (result) {
        event_result_success(result);
    }
    return result;
}

EventResult *event_result_create_failure(
    const char *error_message,
    EventChainErrorCode error_code,
    ErrorDetailLevel detail_level
) {
    EventResult *result = malloc(sizeof(EventResult));
    if (result) {
        event_result_failure(result, error_message, error_code, detail_level);
    }
    return result;
}

void sanitize_error_message(
    char *dest,
    const char *src,
    size_t dest_size,
    ErrorDetailLevel level
) {
    if (!dest || dest_size == 0) return;

    if (!src || level == ERROR_DETAIL_MINIMAL) {
        dest[0] = '\0';
        return;
    }

    size_t i;
    for (i = 0; i < dest_size - 1 && src[i]; i++) {
        /* Filter out control characters */
        if (src[i] >= 32 && src[i] <= 126) {
            dest[i] = src[i];
        } else {
            dest[i] = '?';
        }
    }
    dest[i] = '\0';
}

/* ==============================================================================
 * Utility Functions Implementation
 * ==============================================================================
 */

size_t safe_strnlen(const char *str, size_t maxlen) {
    if (!str || maxlen == 0) return 0;

    size_t len = 0;
    while (len < maxlen && str[len]) {
        len++;
    }
    return len;
}

void safe_strncpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;

    if (!src) {
        dest[0] = '\0';
        return;
    }

    size_t i;
    for (i = 0; i < dest_size - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

bool safe_multiply(size_t a, size_t b, size_t *result) {
    if (!result) return false;

    if (a == 0 || b == 0) {
        *result = 0;
        return true;
    }

    if (a > SIZE_MAX / b) {
        return false;  /* Overflow */
    }

    *result = a * b;
    return true;
}

bool safe_add(size_t a, size_t b, size_t *result) {
    if (!result) return false;

    if (a > SIZE_MAX - b) {
        return false;  /* Overflow */
    }

    *result = a + b;
    return true;
}

bool safe_subtract(size_t a, size_t b, size_t *result) {
    if (!result) return false;

    if (b > a) {
        return false;  /* Underflow */
    }

    *result = a - b;
    return true;
}

void secure_zero(void *ptr, size_t len) {
    if (!ptr || len == 0) return;

    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) {
        *p++ = 0;
    }
}

bool is_valid_function_pointer(const void *ptr) {
    return ptr != NULL && (uintptr_t)ptr >= MIN_FUNCTION_POINTER;
}

bool constant_time_strcmp(const char *a, const char *b, size_t max_len) {
    if (!a || !b) return false;

    unsigned char result = 0;
    size_t i;

    for (i = 0; i < max_len; i++) {
        result |= a[i] ^ b[i];
        if (!a[i] || !b[i]) break;
    }

    return result == 0 && a[i] == b[i];
}

/* ==============================================================================
 * Reference Counting Implementation
 * ==============================================================================
 */

RefCountedValue *ref_counted_value_create(void *data, ValueCleanupFunc cleanup) {
    if (cleanup && !is_valid_function_pointer(cleanup)) {
        return NULL;
    }

    RefCountedValue *value = malloc(sizeof(RefCountedValue));
    if (!value) return NULL;

    value->data = data;
    value->cleanup = cleanup;
    ec_atomic_init(&value->ref_count, 1);

    return value;
}

EventChainErrorCode ref_counted_value_retain(RefCountedValue *value) {
    if (!value) return EC_ERROR_NULL_POINTER;

    size_t old_count = ec_atomic_fetch_add(&value->ref_count, 1);
    if (old_count == SIZE_MAX) {
        ec_atomic_fetch_sub(&value->ref_count, 1);
        return EC_ERROR_OVERFLOW;
    }

    return EC_SUCCESS;
}

EventChainErrorCode ref_counted_value_release(RefCountedValue *value) {
    if (!value) return EC_ERROR_NULL_POINTER;

    size_t old_count = ec_atomic_fetch_sub(&value->ref_count, 1);

    if (old_count == 1) {
        /* Last reference, cleanup */
        if (value->cleanup && value->data) {
            value->cleanup(value->data);
        }
        free(value);
    }

    return EC_SUCCESS;
}

void *ref_counted_value_get_data(const RefCountedValue *value) {
    return value ? value->data : NULL;
}

size_t ref_counted_value_get_count(const RefCountedValue *value) {
    return value ? ec_atomic_load(&value->ref_count) : 0;
}

/* ==============================================================================
 * Context Implementation
 * ==============================================================================
 */

EventContext *event_context_create(void) {
    EventContext *ctx = malloc(sizeof(EventContext));
    if (!ctx) return NULL;

    ctx->entries = malloc(INITIAL_CAPACITY * sizeof(ContextEntry));
    if (!ctx->entries) {
        free(ctx);
        return NULL;
    }

    ctx->count = 0;
    ctx->capacity = INITIAL_CAPACITY;
    ctx->total_memory_bytes = sizeof(EventContext) +
                              (INITIAL_CAPACITY * sizeof(ContextEntry));

    if (ec_mutex_init(&ctx->mutex) != 0) {
        free(ctx->entries);
        free(ctx);
        return NULL;
    }

    return ctx;
}

void event_context_destroy(EventContext *context) {
    if (!context) return;

    ec_mutex_lock(&context->mutex);

    /* Release all values */
    for (size_t i = 0; i < context->count; i++) {
        free(context->entries[i].key);
        ref_counted_value_release(context->entries[i].value);
    }

    free(context->entries);
    ec_mutex_unlock(&context->mutex);
    ec_mutex_destroy(&context->mutex);
    free(context);
}

static int find_entry(EventContext *context, const char *key) {
    for (size_t i = 0; i < context->count; i++) {
        if (strcmp(context->entries[i].key, key) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static EventChainErrorCode ensure_capacity(EventContext *context) {
    if (context->count < context->capacity) {
        return EC_SUCCESS;
    }

    if (context->count >= EVENTCHAINS_MAX_CONTEXT_ENTRIES) {
        return EC_ERROR_CAPACITY_EXCEEDED;
    }

    size_t new_capacity = context->capacity * GROWTH_FACTOR;
    if (new_capacity > EVENTCHAINS_MAX_CONTEXT_ENTRIES) {
        new_capacity = EVENTCHAINS_MAX_CONTEXT_ENTRIES;
    }

    size_t new_size;
    if (!safe_multiply(new_capacity, sizeof(ContextEntry), &new_size)) {
        return EC_ERROR_OVERFLOW;
    }

    ContextEntry *new_entries = realloc(context->entries, new_size);
    if (!new_entries) {
        return EC_ERROR_OUT_OF_MEMORY;
    }

    context->entries = new_entries;
    context->capacity = new_capacity;

    return EC_SUCCESS;
}

EventChainErrorCode event_context_set_with_cleanup(
    EventContext *context,
    const char *key,
    void *value,
    ValueCleanupFunc cleanup
) {
    if (!context || !key) return EC_ERROR_NULL_POINTER;

    size_t key_len = safe_strnlen(key, EVENTCHAINS_MAX_KEY_LENGTH + 1);
    if (key_len == 0 || key_len > EVENTCHAINS_MAX_KEY_LENGTH) {
        return EC_ERROR_KEY_TOO_LONG;
    }

    ec_mutex_lock(&context->mutex);

    /* Check memory limit */
    size_t additional_memory = key_len + 1 + sizeof(RefCountedValue);
    if (context->total_memory_bytes + additional_memory >
        EVENTCHAINS_MAX_CONTEXT_MEMORY) {
        ec_mutex_unlock(&context->mutex);
        return EC_ERROR_MEMORY_LIMIT_EXCEEDED;
    }

    /* Check if key exists */
    int idx = find_entry(context, key);

    /* Create ref-counted value */
    RefCountedValue *ref_value = ref_counted_value_create(value, cleanup);
    if (!ref_value) {
        ec_mutex_unlock(&context->mutex);
        return EC_ERROR_OUT_OF_MEMORY;
    }

    if (idx >= 0) {
        /* Update existing entry */
        ref_counted_value_release(context->entries[idx].value);
        context->entries[idx].value = ref_value;
    } else {
        /* Add new entry */
        EventChainErrorCode err = ensure_capacity(context);
        if (err != EC_SUCCESS) {
            ref_counted_value_release(ref_value);
            ec_mutex_unlock(&context->mutex);
            return err;
        }

        char *key_copy = strdup(key);
        if (!key_copy) {
            ref_counted_value_release(ref_value);
            ec_mutex_unlock(&context->mutex);
            return EC_ERROR_OUT_OF_MEMORY;
        }

        context->entries[context->count].key = key_copy;
        context->entries[context->count].value = ref_value;
        context->count++;
        context->total_memory_bytes += additional_memory;
    }

    ec_mutex_unlock(&context->mutex);
    return EC_SUCCESS;
}

EventChainErrorCode event_context_set(
    EventContext *context,
    const char *key,
    void *value
) {
    return event_context_set_with_cleanup(context, key, value, NULL);
}

EventChainErrorCode event_context_get(
    const EventContext *context,
    const char *key,
    void **value_out
) {
    if (!context || !key || !value_out) return EC_ERROR_NULL_POINTER;

    ec_mutex_lock((ec_mutex_t *)&context->mutex);

    int idx = find_entry((EventContext *)context, key);
    if (idx < 0) {
        ec_mutex_unlock((ec_mutex_t *)&context->mutex);
        return EC_ERROR_NOT_FOUND;
    }

    *value_out = ref_counted_value_get_data(context->entries[idx].value);

    ec_mutex_unlock((ec_mutex_t *)&context->mutex);
    return EC_SUCCESS;
}

EventChainErrorCode event_context_get_ref(
    EventContext *context,
    const char *key,
    RefCountedValue **value_out
) {
    if (!context || !key || !value_out) return EC_ERROR_NULL_POINTER;

    ec_mutex_lock(&context->mutex);

    int idx = find_entry(context, key);
    if (idx < 0) {
        ec_mutex_unlock(&context->mutex);
        return EC_ERROR_NOT_FOUND;
    }

    RefCountedValue *value = context->entries[idx].value;
    ref_counted_value_retain(value);
    *value_out = value;

    ec_mutex_unlock(&context->mutex);
    return EC_SUCCESS;
}

bool event_context_has(
    const EventContext *context,
    const char *key,
    bool constant_time
) {
    if (!context || !key) return false;

    ec_mutex_lock((ec_mutex_t *)&context->mutex);

    bool found;
    if (constant_time) {
        found = false;
        for (size_t i = 0; i < context->count; i++) {
            if (constant_time_strcmp(context->entries[i].key, key,
                                    EVENTCHAINS_MAX_KEY_LENGTH)) {
                found = true;
            }
        }
    } else {
        found = find_entry((EventContext *)context, key) >= 0;
    }

    ec_mutex_unlock((ec_mutex_t *)&context->mutex);
    return found;
}

EventChainErrorCode event_context_remove(EventContext *context, const char *key) {
    if (!context || !key) return EC_ERROR_NULL_POINTER;

    ec_mutex_lock(&context->mutex);

    int idx = find_entry(context, key);
    if (idx < 0) {
        ec_mutex_unlock(&context->mutex);
        return EC_ERROR_NOT_FOUND;
    }

    /* Free key and release value */
    free(context->entries[idx].key);
    ref_counted_value_release(context->entries[idx].value);

    /* Shift remaining entries */
    for (size_t i = idx; i < context->count - 1; i++) {
        context->entries[i] = context->entries[i + 1];
    }
    context->count--;

    ec_mutex_unlock(&context->mutex);
    return EC_SUCCESS;
}

size_t event_context_count(const EventContext *context) {
    if (!context) return 0;

    ec_mutex_lock((ec_mutex_t *)&context->mutex);
    size_t count = context->count;
    ec_mutex_unlock((ec_mutex_t *)&context->mutex);

    return count;
}

size_t event_context_memory_usage(const EventContext *context) {
    if (!context) return 0;

    ec_mutex_lock((ec_mutex_t *)&context->mutex);
    size_t memory = context->total_memory_bytes;
    ec_mutex_unlock((ec_mutex_t *)&context->mutex);

    return memory;
}

void event_context_clear(EventContext *context) {
    if (!context) return;

    ec_mutex_lock(&context->mutex);

    for (size_t i = 0; i < context->count; i++) {
        free(context->entries[i].key);
        ref_counted_value_release(context->entries[i].value);
    }

    context->count = 0;
    context->total_memory_bytes = sizeof(EventContext) +
                                  (context->capacity * sizeof(ContextEntry));

    ec_mutex_unlock(&context->mutex);
}

/* ==============================================================================
 * Events Implementation
 * ==============================================================================
 */

ChainableEvent *chainable_event_create(
    EventExecuteFunc execute,
    void *user_data,
    const char *name
) {
    if (!is_valid_function_pointer(execute)) {
        return NULL;
    }

    ChainableEvent *event = malloc(sizeof(ChainableEvent));
    if (!event) return NULL;

    event->execute = execute;
    event->user_data = user_data;

    if (name) {
        safe_strncpy(event->name, name, EVENTCHAINS_MAX_NAME_LENGTH);
    } else {
        safe_strncpy(event->name, "UnnamedEvent", EVENTCHAINS_MAX_NAME_LENGTH);
    }

    return event;
}

void chainable_event_destroy(ChainableEvent *event) {
    free(event);
}

const char *chainable_event_get_name(const ChainableEvent *event) {
    return event ? event->name : NULL;
}

void *chainable_event_get_user_data(const ChainableEvent *event) {
    return event ? event->user_data : NULL;
}

void chainable_event_set_user_data(ChainableEvent *event, void *user_data) {
    if (event) {
        event->user_data = user_data;
    }
}

/* ==============================================================================
 * Middleware Implementation
 * ==============================================================================
 */

EventMiddleware *event_middleware_create(
    MiddlewareExecuteFunc execute,
    void *user_data,
    const char *name
) {
    if (!is_valid_function_pointer(execute)) {
        return NULL;
    }

    EventMiddleware *middleware = malloc(sizeof(EventMiddleware));
    if (!middleware) return NULL;

    middleware->execute = execute;
    middleware->user_data = user_data;

    if (name) {
        safe_strncpy(middleware->name, name, EVENTCHAINS_MAX_NAME_LENGTH);
    } else {
        safe_strncpy(middleware->name, "UnnamedMiddleware",
                    EVENTCHAINS_MAX_NAME_LENGTH);
    }

    return middleware;
}

void event_middleware_destroy(EventMiddleware *middleware) {
    free(middleware);
}

/* Forward declaration for middleware execution */
static void execute_next_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void *next_data
);

typedef struct MiddlewareContext {
    EventChain *chain;
    size_t current_index;
} MiddlewareContext;

static void execute_event_direct(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void *next_data
) {
    (void)next_data;  /* Unused */

    if (!event || !event->execute) {
        event_result_failure(result_ptr, "Invalid event",
                           EC_ERROR_INVALID_FUNCTION_POINTER,
                           ERROR_DETAIL_FULL);
        return;
    }

    *result_ptr = event->execute(context, event->user_data);
}

static void execute_next_middleware(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void *next_data
) {
    MiddlewareContext *mw_ctx = (MiddlewareContext *)next_data;

    if (mw_ctx->current_index >= mw_ctx->chain->middleware_count) {
        /* No more middleware, execute event */
        execute_event_direct(result_ptr, event, context, NULL);
        return;
    }

    /* Execute current middleware */
    EventMiddleware *middleware =
        mw_ctx->chain->middlewares[mw_ctx->current_index];

    MiddlewareContext next_ctx = *mw_ctx;
    next_ctx.current_index++;

    middleware->execute(
        result_ptr,
        event,
        context,
        execute_next_middleware,
        &next_ctx,
        middleware->user_data
    );
}

void execute_event_with_middleware(
    EventChain *chain,
    ChainableEvent *event,
    EventResult *result_ptr
) {
    if (!chain || !event || !result_ptr) {
        if (result_ptr) {
            event_result_failure(result_ptr, "NULL pointer",
                               EC_ERROR_NULL_POINTER, ERROR_DETAIL_FULL);
        }
        return;
    }

    if (chain->middleware_count == 0) {
        /* No middleware, execute directly */
        execute_event_direct(result_ptr, event, chain->context, NULL);
    } else {
        /* Start middleware pipeline */
        MiddlewareContext mw_ctx = {
            .chain = chain,
            .current_index = 0
        };
        execute_next_middleware(result_ptr, event, chain->context, &mw_ctx);
    }
}

/* ==============================================================================
 * Chain Implementation
 * ==============================================================================
 */

EventChain *event_chain_create(FaultToleranceMode mode) {
    return event_chain_create_with_detail(mode, ERROR_DETAIL_FULL);
}

EventChain *event_chain_create_with_detail(
    FaultToleranceMode mode,
    ErrorDetailLevel detail_level
) {
    EventChain *chain = malloc(sizeof(EventChain));
    if (!chain) return NULL;

    chain->events = NULL;
    chain->event_count = 0;
    chain->event_capacity = 0;

    chain->middlewares = NULL;
    chain->middleware_count = 0;
    chain->middleware_capacity = 0;

    chain->context = event_context_create();
    if (!chain->context) {
        free(chain);
        return NULL;
    }

    chain->fault_tolerance = mode;
    chain->error_detail_level = detail_level;
    chain->failure_handler = NULL;
    chain->failure_handler_data = NULL;
    ec_atomic_init(&chain->is_executing, 0);
    ec_atomic_init(&chain->signal_interrupted, 0);

    return chain;
}

void event_chain_destroy(EventChain *chain) {
    if (!chain) return;

    /* Free all events */
    for (size_t i = 0; i < chain->event_count; i++) {
        chainable_event_destroy(chain->events[i]);
    }
    free(chain->events);

    /* Free all middleware */
    for (size_t i = 0; i < chain->middleware_count; i++) {
        event_middleware_destroy(chain->middlewares[i]);
    }
    free(chain->middlewares);

    /* Free context */
    event_context_destroy(chain->context);

    free(chain);
}

static EventChainErrorCode ensure_event_capacity(EventChain *chain) {
    if (chain->event_count < chain->event_capacity) {
        return EC_SUCCESS;
    }

    if (chain->event_count >= EVENTCHAINS_MAX_EVENTS) {
        return EC_ERROR_CAPACITY_EXCEEDED;
    }

    size_t new_capacity = chain->event_capacity == 0 ?
                         INITIAL_CAPACITY :
                         chain->event_capacity * GROWTH_FACTOR;

    if (new_capacity > EVENTCHAINS_MAX_EVENTS) {
        new_capacity = EVENTCHAINS_MAX_EVENTS;
    }

    size_t new_size;
    if (!safe_multiply(new_capacity, sizeof(ChainableEvent *), &new_size)) {
        return EC_ERROR_OVERFLOW;
    }

    ChainableEvent **new_events = realloc(chain->events, new_size);
    if (!new_events) {
        return EC_ERROR_OUT_OF_MEMORY;
    }

    chain->events = new_events;
    chain->event_capacity = new_capacity;

    return EC_SUCCESS;
}

EventChainErrorCode event_chain_add_event(
    EventChain *chain,
    ChainableEvent *event
) {
    if (!chain || !event) return EC_ERROR_NULL_POINTER;

    if (ec_atomic_load(&chain->is_executing)) {
        return EC_ERROR_REENTRANCY;
    }

    EventChainErrorCode err = ensure_event_capacity(chain);
    if (err != EC_SUCCESS) return err;

    chain->events[chain->event_count++] = event;
    return EC_SUCCESS;
}

static EventChainErrorCode ensure_middleware_capacity(EventChain *chain) {
    if (chain->middleware_count < chain->middleware_capacity) {
        return EC_SUCCESS;
    }

    if (chain->middleware_count >= EVENTCHAINS_MAX_MIDDLEWARE) {
        return EC_ERROR_CAPACITY_EXCEEDED;
    }

    size_t new_capacity = chain->middleware_capacity == 0 ?
                         INITIAL_CAPACITY :
                         chain->middleware_capacity * GROWTH_FACTOR;

    if (new_capacity > EVENTCHAINS_MAX_MIDDLEWARE) {
        new_capacity = EVENTCHAINS_MAX_MIDDLEWARE;
    }

    size_t new_size;
    if (!safe_multiply(new_capacity, sizeof(EventMiddleware *), &new_size)) {
        return EC_ERROR_OVERFLOW;
    }

    EventMiddleware **new_middlewares = realloc(chain->middlewares, new_size);
    if (!new_middlewares) {
        return EC_ERROR_OUT_OF_MEMORY;
    }

    chain->middlewares = new_middlewares;
    chain->middleware_capacity = new_capacity;

    return EC_SUCCESS;
}

EventChainErrorCode event_chain_use_middleware(
    EventChain *chain,
    EventMiddleware *middleware
) {
    if (!chain || !middleware) return EC_ERROR_NULL_POINTER;

    if (ec_atomic_load(&chain->is_executing)) {
        return EC_ERROR_REENTRANCY;
    }

    EventChainErrorCode err = ensure_middleware_capacity(chain);
    if (err != EC_SUCCESS) return err;

    chain->middlewares[chain->middleware_count++] = middleware;
    return EC_SUCCESS;
}

EventChainErrorCode event_chain_set_failure_handler(
    EventChain *chain,
    FailureHandlerFunc handler,
    void *user_data
) {
    if (!chain) return EC_ERROR_NULL_POINTER;

    chain->failure_handler = handler;
    chain->failure_handler_data = user_data;

    return EC_SUCCESS;
}

EventContext *event_chain_get_context(EventChain *chain) {
    return chain ? chain->context : NULL;
}

void event_chain_execute(EventChain *chain, ChainResult *result_ptr) {
    if (!chain || !result_ptr) {
        if (result_ptr) {
            result_ptr->success = false;
            result_ptr->failures = NULL;
            result_ptr->failure_count = 0;
        }
        return;
    }

    /* Check for reentrancy */
    int expected = 0;
    if (!ec_atomic_compare_exchange_strong(&chain->is_executing, &expected, 1)) {
        result_ptr->success = false;
        result_ptr->failures = NULL;
        result_ptr->failure_count = 0;
        return;
    }

    /* Initialize result */
    result_ptr->success = true;
    result_ptr->failures = NULL;
    result_ptr->failure_count = 0;

    size_t failure_capacity = 0;

    /* Execute each event */
    for (size_t i = 0; i < chain->event_count; i++) {
        EventResult event_result;

        execute_event_with_middleware(
            chain,
            chain->events[i],
            &event_result
        );

        if (!event_result.success) {
            /* Handle failure based on fault tolerance mode */
            bool should_continue = false;

            switch (chain->fault_tolerance) {
                case FAULT_TOLERANCE_STRICT:
                    should_continue = false;
                    break;

                case FAULT_TOLERANCE_LENIENT:
                case FAULT_TOLERANCE_BEST_EFFORT:
                    should_continue = true;
                    break;

                case FAULT_TOLERANCE_CUSTOM:
                    if (chain->failure_handler) {
                        should_continue = chain->failure_handler(
                            chain,
                            chain->events[i],
                            &event_result,
                            chain->failure_handler_data
                        );
                    } else {
                        should_continue = false;
                    }
                    break;
            }

            /* Record failure */
            if (result_ptr->failure_count >= failure_capacity) {
                failure_capacity = failure_capacity == 0 ? 4 : failure_capacity * 2;
                FailureInfo *new_failures = realloc(
                    result_ptr->failures,
                    failure_capacity * sizeof(FailureInfo)
                );
                if (new_failures) {
                    result_ptr->failures = new_failures;
                } else {
                    /* Out of memory, can't record failure */
                    should_continue = false;
                }
            }

            if (result_ptr->failures) {
                FailureInfo *failures = (FailureInfo *)result_ptr->failures;
                FailureInfo *failure = &failures[result_ptr->failure_count++];
                safe_strncpy(failure->event_name, chain->events[i]->name,
                           EVENTCHAINS_MAX_NAME_LENGTH);
                safe_strncpy(failure->error_message, event_result.error_message,
                           EVENTCHAINS_MAX_ERROR_LENGTH);
                failure->error_code = event_result.error_code;
            }

            if (!should_continue) {
                result_ptr->success = false;
                break;
            }
        }
    }

    /* Mark execution as complete */
    ec_atomic_store(&chain->is_executing, 0);

    /* Update result based on failure count */
    if (result_ptr->failure_count > 0 &&
        chain->fault_tolerance == FAULT_TOLERANCE_STRICT) {
        result_ptr->success = false;
    }
}

int event_chain_was_interrupted(EventChain *chain) {
    return chain ? ec_atomic_load(&chain->signal_interrupted) : 0;
}

void chain_result_destroy(ChainResult *result) {
    if (!result) return;

    free(result->failures);
    result->failures = NULL;
    result->failure_count = 0;
}