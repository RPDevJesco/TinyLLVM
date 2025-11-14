/**
 * ==============================================================================
 * EventChains - High-Performance Event Processing Library
 * ==============================================================================
 * Version: 3.1.0
 * Architecture: x86_64 (AMD64)
 * Implementation: NASM Assembly
 *
 * EventChains is a high-performance event processing library implemented in
 * x86_64 assembly language. It provides a robust framework for building
 * event-driven applications with support for middleware, context management,
 * and sophisticated error handling.
 *
 * Key Features:
 *   - Zero-overhead event chain execution
 *   - Reference-counted context management
 *   - Configurable fault tolerance modes
 *   - Thread-safe operations
 *   - Memory-efficient design
 *   - Comprehensive error reporting
 *
 * Copyright (c) 2025 EventChains Project
 * Licensed under the MIT License
 * ==============================================================================
 */

#ifndef EVENTCHAINS_H
#define EVENTCHAINS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "eventchains_platform.h"

/* ==============================================================================
 * Version Information
 * ==============================================================================
 */

#define EVENTCHAINS_VERSION_MAJOR 3
#define EVENTCHAINS_VERSION_MINOR 1
#define EVENTCHAINS_VERSION_PATCH 0
#define EVENTCHAINS_VERSION_STRING "3.1.0"

/* ==============================================================================
 * Configuration Constants
 * ==============================================================================
 */

/* Maximum number of events in a chain */
#define EVENTCHAINS_MAX_EVENTS 1024

/* Maximum number of middleware in a chain */
#define EVENTCHAINS_MAX_MIDDLEWARE 16

/* Maximum number of context entries */
#define EVENTCHAINS_MAX_CONTEXT_ENTRIES 512

/* Maximum context memory (10 MB) */
#define EVENTCHAINS_MAX_CONTEXT_MEMORY 10485760

/* Maximum length for event/middleware names */
#define EVENTCHAINS_MAX_NAME_LENGTH 256

/* Maximum length for context keys */
#define EVENTCHAINS_MAX_KEY_LENGTH 256

/* Maximum length for error messages */
#define EVENTCHAINS_MAX_ERROR_LENGTH 1024

/* ==============================================================================
 * Error Codes
 * ==============================================================================
 */

typedef enum {
    EC_SUCCESS = 0,                          /* Operation succeeded */
    EC_ERROR_NULL_POINTER = 1,               /* NULL pointer provided */
    EC_ERROR_INVALID_PARAMETER = 2,          /* Invalid parameter */
    EC_ERROR_OUT_OF_MEMORY = 3,              /* Memory allocation failed */
    EC_ERROR_CAPACITY_EXCEEDED = 4,          /* Maximum capacity reached */
    EC_ERROR_KEY_TOO_LONG = 5,               /* Context key exceeds max length */
    EC_ERROR_NAME_TOO_LONG = 6,              /* Name exceeds max length */
    EC_ERROR_NOT_FOUND = 7,                  /* Item not found */
    EC_ERROR_OVERFLOW = 8,                   /* Arithmetic overflow */
    EC_ERROR_EVENT_EXECUTION_FAILED = 9,     /* Event execution failed */
    EC_ERROR_MIDDLEWARE_FAILED = 10,         /* Middleware execution failed */
    EC_ERROR_REENTRANCY = 11,                /* Reentrancy detected */
    EC_ERROR_MEMORY_LIMIT_EXCEEDED = 12,     /* Memory limit exceeded */
    EC_ERROR_INVALID_FUNCTION_POINTER = 13,  /* Invalid function pointer */
    EC_ERROR_TIME_CONVERSION = 14,           /* Time conversion error */
    EC_ERROR_SIGNAL_INTERRUPTED = 15         /* Signal interrupted operation */
} EventChainErrorCode;

/* ==============================================================================
 * Fault Tolerance Modes
 * ==============================================================================
 */

typedef enum {
    FAULT_TOLERANCE_STRICT = 0,      /* Stop on first error */
    FAULT_TOLERANCE_LENIENT = 1,     /* Log errors but continue */
    FAULT_TOLERANCE_BEST_EFFORT = 2, /* Ignore all errors */
    FAULT_TOLERANCE_CUSTOM = 3       /* Use custom failure handler */
} FaultToleranceMode;

/* ==============================================================================
 * Error Detail Levels
 * ==============================================================================
 */

typedef enum {
    ERROR_DETAIL_FULL = 0,     /* Full error messages */
    ERROR_DETAIL_MINIMAL = 1   /* Minimal error information */
} ErrorDetailLevel;

/* ==============================================================================
 * Function Type Definitions
 * ==============================================================================
 */

/**
 * ValueCleanupFunc - Cleanup function for context values
 *
 * @param value  The value to clean up
 */
typedef void (*ValueCleanupFunc)(void *value);

/**
 * EventExecuteFunc - Event execution function signature
 *
 * Forward declare context and result types
 */
typedef struct EventContext EventContext;
typedef struct EventResult EventResult;
typedef struct ChainableEvent ChainableEvent;

typedef EventResult (*EventExecuteFunc)(EventContext *context, void *user_data);

/**
 * MiddlewareExecuteFunc - Middleware execution function signature
 */
typedef void (*MiddlewareExecuteFunc)(
    EventResult *result_ptr,
    ChainableEvent *event,
    EventContext *context,
    void (*next)(EventResult *, ChainableEvent *, EventContext *, void *),
    void *next_data,
    void *user_data
);

/**
 * FailureHandlerFunc - Custom failure handler for FAULT_TOLERANCE_CUSTOM mode
 */
typedef struct EventChain EventChain;

typedef bool (*FailureHandlerFunc)(
    EventChain *chain,
    ChainableEvent *event,
    EventResult *result,
    void *user_data
);

/* ==============================================================================
 * Forward Declarations (complete list)
 * ==============================================================================
 */

typedef struct EventMiddleware EventMiddleware;
typedef struct RefCountedValue RefCountedValue;
typedef struct ChainResult ChainResult;

/* ==============================================================================
 * Core Structures
 * ==============================================================================
 */

/**
 * RefCountedValue - Reference-counted wrapper for context values
 */
struct RefCountedValue {
    void *data;
    ValueCleanupFunc cleanup;
    ec_atomic_size_t ref_count;
};

/**
 * ContextEntry - Single key-value pair in context
 */
typedef struct ContextEntry {
    char *key;
    RefCountedValue *value;
} ContextEntry;

/**
 * EventContext - Thread-safe key-value storage
 */
struct EventContext {
    ContextEntry *entries;
    size_t count;
    size_t capacity;
    size_t total_memory_bytes;
    ec_mutex_t mutex;
};

/**
 * ChainableEvent - A single event in a chain
 */
struct ChainableEvent {
    EventExecuteFunc execute;
    void *user_data;
    char name[EVENTCHAINS_MAX_NAME_LENGTH];
};

/**
 * EventMiddleware - Middleware layer for event processing
 */
struct EventMiddleware {
    MiddlewareExecuteFunc execute;
    void *user_data;
    char name[EVENTCHAINS_MAX_NAME_LENGTH];
};

/**
 * EventChain - Collection of events with middleware
 */
struct EventChain {
    ChainableEvent **events;
    size_t event_count;
    size_t event_capacity;

    EventMiddleware **middlewares;
    size_t middleware_count;
    size_t middleware_capacity;

    EventContext *context;
    FaultToleranceMode fault_tolerance;
    ErrorDetailLevel error_detail_level;
    FailureHandlerFunc failure_handler;
    void *failure_handler_data;
    ec_atomic_int is_executing;
    ec_atomic_int signal_interrupted;
};

/**
 * FailureInfo - Information about a failed event
 */
typedef struct FailureInfo {
    char event_name[EVENTCHAINS_MAX_NAME_LENGTH];
    char error_message[EVENTCHAINS_MAX_ERROR_LENGTH];
    EventChainErrorCode error_code;
} FailureInfo;

/**
 * EventResult - Result of an event execution
 *
 * This structure is returned by event execution functions to indicate
 * success or failure and provide error details.
 */
struct EventResult {
    bool success;                              /* Whether the event succeeded */
    char _padding[7];                          /* Alignment padding */
    char error_message[EVENTCHAINS_MAX_ERROR_LENGTH]; /* Error description */
    EventChainErrorCode error_code;            /* Error code */
    char _padding_end[4];                      /* Final alignment padding */
};

/**
 * ChainResult - Result of a chain execution
 *
 * Contains the overall success status and information about any failures
 * that occurred during chain execution.
 */
struct ChainResult {
    bool success;          /* Whether the entire chain succeeded */
    void *failures;        /* Array of failure information */
    size_t failure_count;  /* Number of failures that occurred */
};

/* ==============================================================================
 * Core Module - Library Information and Initialization
 * ==============================================================================
 */

/**
 * Get the version string (e.g., "3.1.0")
 * @return Version string
 */
const char *event_chain_version_string(void);

/**
 * Get the version numbers
 * @param major  Pointer to store major version (can be NULL)
 * @param minor  Pointer to store minor version (can be NULL)
 * @param patch  Pointer to store patch version (can be NULL)
 */
void event_chain_version_numbers(int *major, int *minor, int *patch);

/**
 * Get detailed build information
 * @return Multi-line string with build details
 */
const char *event_chain_build_info(void);

/**
 * Get architecture information
 * @return Architecture description string
 */
const char *event_chain_architecture_info(void);

/**
 * Get list of available features
 * @return Multi-line string listing features
 */
const char *event_chain_features(void);

/**
 * Get copyright information
 * @return Copyright notice string
 */
const char *event_chain_copyright(void);

/**
 * Get human-readable error string for an error code
 * @param code  Error code
 * @return      Error description string
 */
const char *event_chain_error_string(EventChainErrorCode code);

/**
 * Get performance statistics array
 * @return Pointer to array of 8 uint64_t counters
 */
const uint64_t *event_chain_get_perf_stats(void);

/**
 * Reset all performance statistics to zero
 */
void event_chain_reset_perf_stats(void);

/**
 * Get maximum number of events per chain
 * @return Maximum event count
 */
size_t event_chain_get_max_events(void);

/**
 * Get maximum number of middleware per chain
 * @return Maximum middleware count
 */
size_t event_chain_get_max_middleware(void);

/**
 * Get maximum number of context entries
 * @return Maximum context entry count
 */
size_t event_chain_get_max_context_entries(void);

/**
 * Get maximum context memory in bytes
 * @return Maximum context memory
 */
size_t event_chain_get_max_context_memory(void);

/**
 * Initialize the EventChains library
 * Call this before using any other functions
 */
void event_chain_initialize(void);

/**
 * Clean up the EventChains library
 * Call this when done using the library
 */
void event_chain_cleanup(void);

/* ==============================================================================
 * Error Module - Error Result Management
 * ==============================================================================
 */

/**
 * Initialize an EventResult as success
 * @param result  Pointer to EventResult to initialize
 */
void event_result_success(EventResult *result);

/**
 * Initialize an EventResult as failure
 * @param result         Pointer to EventResult to initialize
 * @param error_message  Error message (can be NULL)
 * @param error_code     Error code
 * @param detail_level   Level of error detail
 */
void event_result_failure(
    EventResult *result,
    const char *error_message,
    EventChainErrorCode error_code,
    ErrorDetailLevel detail_level
);

/**
 * Create a success EventResult (heap-allocated)
 * @return Pointer to new EventResult (must be freed by caller)
 */
EventResult *event_result_create_success(void);

/**
 * Create a failure EventResult (heap-allocated)
 * @param error_message  Error message (can be NULL)
 * @param error_code     Error code
 * @param detail_level   Level of error detail
 * @return Pointer to new EventResult (must be freed by caller)
 */
EventResult *event_result_create_failure(
    const char *error_message,
    EventChainErrorCode error_code,
    ErrorDetailLevel detail_level
);

/**
 * Sanitize an error message for safe output
 * @param dest        Destination buffer
 * @param src         Source error message
 * @param dest_size   Size of destination buffer
 * @param level       Detail level for sanitization
 */
void sanitize_error_message(
    char *dest,
    const char *src,
    size_t dest_size,
    ErrorDetailLevel level
);

/* ==============================================================================
 * Reference Counting Module
 * ==============================================================================
 */

/**
 * Create a reference-counted value
 * @param data     Pointer to data to wrap
 * @param cleanup  Cleanup function (can be NULL)
 * @return         Pointer to RefCountedValue, or NULL on error
 */
RefCountedValue *ref_counted_value_create(void *data, ValueCleanupFunc cleanup);

/**
 * Increment the reference count
 * @param value  Pointer to RefCountedValue
 * @return       EC_SUCCESS or error code
 */
EventChainErrorCode ref_counted_value_retain(RefCountedValue *value);

/**
 * Decrement the reference count (frees if count reaches 0)
 * @param value  Pointer to RefCountedValue
 * @return       EC_SUCCESS or error code
 */
EventChainErrorCode ref_counted_value_release(RefCountedValue *value);

/**
 * Get the data pointer from a RefCountedValue
 * @param value  Pointer to RefCountedValue
 * @return       Data pointer, or NULL if value is NULL
 */
void *ref_counted_value_get_data(const RefCountedValue *value);

/**
 * Get the current reference count
 * @param value  Pointer to RefCountedValue
 * @return       Current reference count, or 0 if value is NULL
 */
size_t ref_counted_value_get_count(const RefCountedValue *value);

/* ==============================================================================
 * Context Module - Key-Value Storage
 * ==============================================================================
 */

/**
 * Create a new EventContext
 * @return Pointer to new EventContext, or NULL on error
 */
EventContext *event_context_create(void);

/**
 * Destroy an EventContext and free all resources
 * @param context  Pointer to EventContext
 */
void event_context_destroy(EventContext *context);

/**
 * Set a key-value pair in the context (no cleanup function)
 * @param context  Pointer to EventContext
 * @param key      Key string
 * @param value    Value pointer
 * @return         EC_SUCCESS or error code
 */
EventChainErrorCode event_context_set(
    EventContext *context,
    const char *key,
    void *value
);

/**
 * Set a key-value pair in the context with cleanup function
 * @param context  Pointer to EventContext
 * @param key      Key string
 * @param value    Value pointer
 * @param cleanup  Cleanup function to call when value is removed
 * @return         EC_SUCCESS or error code
 */
EventChainErrorCode event_context_set_with_cleanup(
    EventContext *context,
    const char *key,
    void *value,
    ValueCleanupFunc cleanup
);

/**
 * Get a value from the context (returns raw pointer)
 * @param context    Pointer to EventContext
 * @param key        Key string
 * @param value_out  Pointer to store retrieved value
 * @return           EC_SUCCESS or error code
 */
EventChainErrorCode event_context_get(
    const EventContext *context,
    const char *key,
    void **value_out
);

/**
 * Get a reference-counted value from the context
 * @param context    Pointer to EventContext
 * @param key        Key string
 * @param value_out  Pointer to store RefCountedValue
 * @return           EC_SUCCESS or error code
 */
EventChainErrorCode event_context_get_ref(
    EventContext *context,
    const char *key,
    RefCountedValue **value_out
);

/**
 * Check if a key exists in the context
 * @param context        Pointer to EventContext
 * @param key            Key string
 * @param constant_time  Whether to use constant-time comparison
 * @return               true if key exists, false otherwise
 */
bool event_context_has(
    const EventContext *context,
    const char *key,
    bool constant_time
);

/**
 * Remove a key-value pair from the context
 * @param context  Pointer to EventContext
 * @param key      Key string
 * @return         EC_SUCCESS or error code
 */
EventChainErrorCode event_context_remove(EventContext *context, const char *key);

/**
 * Get the number of entries in the context
 * @param context  Pointer to EventContext
 * @return         Number of key-value pairs, or 0 if context is NULL
 */
size_t event_context_count(const EventContext *context);

/**
 * Get the current memory usage of the context
 * @param context  Pointer to EventContext
 * @return         Memory usage in bytes, or 0 if context is NULL
 */
size_t event_context_memory_usage(const EventContext *context);

/**
 * Clear all entries from the context
 * @param context  Pointer to EventContext
 */
void event_context_clear(EventContext *context);

/* ==============================================================================
 * Events Module - Chainable Event Management
 * ==============================================================================
 */

/**
 * Create a new ChainableEvent
 * @param execute    Event execution function
 * @param user_data  User data to pass to execution function
 * @param name       Event name (can be NULL for default name)
 * @return           Pointer to new ChainableEvent, or NULL on error
 */
ChainableEvent *chainable_event_create(
    EventExecuteFunc execute,
    void *user_data,
    const char *name
);

/**
 * Destroy a ChainableEvent
 * @param event  Pointer to ChainableEvent
 */
void chainable_event_destroy(ChainableEvent *event);

/**
 * Get the name of a ChainableEvent
 * @param event  Pointer to ChainableEvent
 * @return       Event name string, or NULL if event is NULL
 */
const char *chainable_event_get_name(const ChainableEvent *event);

/**
 * Get the user data from a ChainableEvent
 * @param event  Pointer to ChainableEvent
 * @return       User data pointer, or NULL if event is NULL
 */
void *chainable_event_get_user_data(const ChainableEvent *event);

/**
 * Set the user data for a ChainableEvent
 * @param event      Pointer to ChainableEvent
 * @param user_data  New user data pointer
 */
void chainable_event_set_user_data(ChainableEvent *event, void *user_data);

/* ==============================================================================
 * Middleware Module
 * ==============================================================================
 */

/**
 * Create a new EventMiddleware
 * @param execute    Middleware execution function
 * @param user_data  User data to pass to execution function
 * @param name       Middleware name (can be NULL for default name)
 * @return           Pointer to new EventMiddleware, or NULL on error
 */
EventMiddleware *event_middleware_create(
    MiddlewareExecuteFunc execute,
    void *user_data,
    const char *name
);

/**
 * Destroy an EventMiddleware
 * @param middleware  Pointer to EventMiddleware
 */
void event_middleware_destroy(EventMiddleware *middleware);

/**
 * Execute an event with middleware pipeline
 * @param chain       Pointer to EventChain
 * @param event       Pointer to ChainableEvent
 * @param result_ptr  Pointer to store result
 */
void execute_event_with_middleware(
    EventChain *chain,
    ChainableEvent *event,
    EventResult *result_ptr
);

/* ==============================================================================
 * Chain Module - Event Chain Management
 * ==============================================================================
 */

/**
 * Create a new EventChain with specified fault tolerance mode
 * @param mode  Fault tolerance mode
 * @return      Pointer to new EventChain, or NULL on error
 */
EventChain *event_chain_create(FaultToleranceMode mode);

/**
 * Create a new EventChain with detailed configuration
 * @param mode          Fault tolerance mode
 * @param detail_level  Error detail level
 * @return              Pointer to new EventChain, or NULL on error
 */
EventChain *event_chain_create_with_detail(
    FaultToleranceMode mode,
    ErrorDetailLevel detail_level
);

/**
 * Destroy an EventChain and free all resources
 * @param chain  Pointer to EventChain
 */
void event_chain_destroy(EventChain *chain);

/**
 * Add an event to the chain
 * @param chain  Pointer to EventChain
 * @param event  Pointer to ChainableEvent (ownership transferred to chain)
 * @return       EC_SUCCESS or error code
 */
EventChainErrorCode event_chain_add_event(EventChain *chain, ChainableEvent *event);

/**
 * Add middleware to the chain
 * @param chain       Pointer to EventChain
 * @param middleware  Pointer to EventMiddleware (ownership transferred to chain)
 * @return            EC_SUCCESS or error code
 */
EventChainErrorCode event_chain_use_middleware(
    EventChain *chain,
    EventMiddleware *middleware
);

/**
 * Set a custom failure handler (for FAULT_TOLERANCE_CUSTOM mode)
 * @param chain      Pointer to EventChain
 * @param handler    Failure handler function
 * @param user_data  User data to pass to handler
 * @return           EC_SUCCESS or error code
 */
EventChainErrorCode event_chain_set_failure_handler(
    EventChain *chain,
    FailureHandlerFunc handler,
    void *user_data
);

/**
 * Get the context from the chain
 * @param chain  Pointer to EventChain
 * @return       Pointer to EventContext, or NULL if chain is NULL
 */
EventContext *event_chain_get_context(EventChain *chain);

/**
 * Execute the entire event chain
 * @param chain       Pointer to EventChain
 * @param result_ptr  Pointer to store ChainResult
 */
void event_chain_execute(EventChain *chain, ChainResult *result_ptr);

/**
 * Check if the chain was interrupted by a signal
 * @param chain  Pointer to EventChain
 * @return       Non-zero if interrupted, 0 otherwise
 */
int event_chain_was_interrupted(EventChain *chain);

/**
 * Destroy a ChainResult and free resources
 * @param result  Pointer to ChainResult
 */
void chain_result_destroy(ChainResult *result);

/* ==============================================================================
 * Utility Functions
 * ==============================================================================
 */

/**
 * Safe string length calculation with maximum limit
 * @param str     String to measure
 * @param maxlen  Maximum length to check
 * @return        Length of string (up to maxlen), or 0 if str is NULL
 */
size_t safe_strnlen(const char *str, size_t maxlen);

/**
 * Safe string copy with guaranteed null termination
 * @param dest       Destination buffer
 * @param src        Source string
 * @param dest_size  Size of destination buffer
 */
void safe_strncpy(char *dest, const char *src, size_t dest_size);

/**
 * Safe multiplication with overflow detection
 * @param a       First operand
 * @param b       Second operand
 * @param result  Pointer to store result
 * @return        true if no overflow, false if overflow detected
 */
bool safe_multiply(size_t a, size_t b, size_t *result);

/**
 * Safe addition with overflow detection
 * @param a       First operand
 * @param b       Second operand
 * @param result  Pointer to store result
 * @return        true if no overflow, false if overflow detected
 */
bool safe_add(size_t a, size_t b, size_t *result);

/**
 * Safe subtraction with underflow detection
 * @param a       First operand
 * @param b       Second operand
 * @param result  Pointer to store result
 * @return        true if no underflow, false if underflow detected
 */
bool safe_subtract(size_t a, size_t b, size_t *result);

/**
 * Securely zero memory (resistant to compiler optimization)
 * @param ptr  Pointer to memory to zero
 * @param len  Length of memory to zero
 */
void secure_zero(void *ptr, size_t len);

/**
 * Check if a pointer is a valid function pointer
 * @param ptr  Pointer to check
 * @return     true if valid, false otherwise
 */
bool is_valid_function_pointer(const void *ptr);

/**
 * Constant-time string comparison
 * @param a        First string
 * @param b        Second string
 * @param max_len  Maximum length to compare
 * @return         true if strings match, false otherwise
 */
bool constant_time_strcmp(const char *a, const char *b, size_t max_len);

/* ==============================================================================
 * End of Header
 * ==============================================================================
 */

#ifdef __cplusplus
}
#endif

#endif /* EVENTCHAINS_H */