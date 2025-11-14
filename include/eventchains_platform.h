/**
 * ==============================================================================
 * EventChains - Platform Abstraction Layer
 * ==============================================================================
 *
 * Provides cross-platform abstractions for:
 * - Atomics (C11 stdatomic.h vs compiler intrinsics)
 * - Threading (POSIX pthread vs Windows threads)
 * - Mutexes (pthread_mutex vs Windows CRITICAL_SECTION)
 *
 * This enables EventChains to work on:
 * - Linux/macOS/BSD (POSIX)
 * - Windows (native Win32 API)
 * - Any C99+ compiler (no C11 required)
 *
 * Copyright (c) 2024 EventChains Project
 * Licensed under the MIT License
 * ==============================================================================
 */

#ifndef EVENTCHAINS_PLATFORM_H
#define EVENTCHAINS_PLATFORM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==============================================================================
 * Platform Detection
 * ==============================================================================
 */

/* Detect Windows */
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define EC_PLATFORM_WINDOWS 1
    #define EC_PLATFORM_POSIX 0
#else
    #define EC_PLATFORM_WINDOWS 0
    #define EC_PLATFORM_POSIX 1
#endif

/* Detect C11 atomics support */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
    #define EC_HAS_C11_ATOMICS 1
#else
    #define EC_HAS_C11_ATOMICS 0
#endif

/* ==============================================================================
 * Atomic Operations Abstraction
 * ==============================================================================
 */

#if EC_HAS_C11_ATOMICS
    /* Use C11 stdatomic.h */
    #include <stdatomic.h>

    typedef _Atomic size_t ec_atomic_size_t;
    typedef _Atomic int ec_atomic_int;
    typedef _Atomic uint64_t ec_atomic_uint64_t;

    #define ec_atomic_init(ptr, val) atomic_init(ptr, val)
    #define ec_atomic_load(ptr) atomic_load(ptr)
    #define ec_atomic_store(ptr, val) atomic_store(ptr, val)
    #define ec_atomic_fetch_add(ptr, val) atomic_fetch_add(ptr, val)
    #define ec_atomic_fetch_sub(ptr, val) atomic_fetch_sub(ptr, val)
    #define ec_atomic_compare_exchange_strong(ptr, expected, desired) \
        atomic_compare_exchange_strong(ptr, expected, desired)

#elif defined(_MSC_VER)
    /* Use MSVC intrinsics */
    #include <intrin.h>

    typedef volatile size_t ec_atomic_size_t;
    typedef volatile int ec_atomic_int;
    typedef volatile uint64_t ec_atomic_uint64_t;

    #define ec_atomic_init(ptr, val) (*(ptr) = (val))
    #define ec_atomic_load(ptr) (*(ptr))
    #define ec_atomic_store(ptr, val) (*(ptr) = (val))

    /* Size-specific intrinsics for MSVC */
    #if defined(_WIN64)
        #define ec_atomic_fetch_add(ptr, val) _InterlockedExchangeAdd64((volatile __int64*)(ptr), (val))
        #define ec_atomic_fetch_sub(ptr, val) _InterlockedExchangeAdd64((volatile __int64*)(ptr), -(val))
    #else
        #define ec_atomic_fetch_add(ptr, val) _InterlockedExchangeAdd((volatile long*)(ptr), (val))
        #define ec_atomic_fetch_sub(ptr, val) _InterlockedExchangeAdd((volatile long*)(ptr), -(val))
    #endif

    static inline bool ec_atomic_compare_exchange_strong(ec_atomic_int *ptr, int *expected, int desired) {
        int old = _InterlockedCompareExchange((volatile long*)ptr, desired, *expected);
        if (old == *expected) return true;
        *expected = old;
        return false;
    }

#elif defined(__GNUC__) || defined(__clang__)
    /* Use GCC/Clang __sync builtins (available since GCC 4.1, works in C99) */
    typedef volatile size_t ec_atomic_size_t;
    typedef volatile int ec_atomic_int;
    typedef volatile uint64_t ec_atomic_uint64_t;

    #define ec_atomic_init(ptr, val) (*(ptr) = (val))
    /* For loads from const pointers, we need to cast away const.
     * This is safe because __sync_fetch_and_add with 0 doesn't modify. */
    #define ec_atomic_load(ptr) __sync_fetch_and_add((ec_atomic_size_t *)(uintptr_t)(ptr), 0)
    #define ec_atomic_store(ptr, val) do { \
        __sync_synchronize(); \
        *(ptr) = (val); \
        __sync_synchronize(); \
    } while(0)
    #define ec_atomic_fetch_add(ptr, val) __sync_fetch_and_add(ptr, val)
    #define ec_atomic_fetch_sub(ptr, val) __sync_fetch_and_sub(ptr, val)

    static inline bool ec_atomic_compare_exchange_strong(ec_atomic_int *ptr, int *expected, int desired) {
        int old = __sync_val_compare_and_swap(ptr, *expected, desired);
        if (old == *expected) return true;
        *expected = old;
        return false;
    }

#else
    /* Fallback: No atomics - use mutex protection */
    #error "No atomic operations available for this compiler. Please use a C11 compiler or GCC/Clang."
#endif

/* ==============================================================================
 * Mutex Abstraction
 * ==============================================================================
 */

#if EC_PLATFORM_POSIX
    /* POSIX threads */
    #include <pthread.h>

    typedef pthread_mutex_t ec_mutex_t;

    #define ec_mutex_init(mutex) pthread_mutex_init(mutex, NULL)
    #define ec_mutex_destroy(mutex) pthread_mutex_destroy(mutex)
    #define ec_mutex_lock(mutex) pthread_mutex_lock(mutex)
    #define ec_mutex_unlock(mutex) pthread_mutex_unlock(mutex)

#elif EC_PLATFORM_WINDOWS
    /* Windows critical sections */
    #include <windows.h>

    typedef CRITICAL_SECTION ec_mutex_t;

    static inline int ec_mutex_init(ec_mutex_t *mutex) {
        InitializeCriticalSection(mutex);
        return 0;
    }

    static inline int ec_mutex_destroy(ec_mutex_t *mutex) {
        DeleteCriticalSection(mutex);
        return 0;
    }

    static inline int ec_mutex_lock(ec_mutex_t *mutex) {
        EnterCriticalSection(mutex);
        return 0;
    }

    static inline int ec_mutex_unlock(ec_mutex_t *mutex) {
        LeaveCriticalSection(mutex);
        return 0;
    }

#else
    #error "Unsupported platform for threading"
#endif

/* ==============================================================================
 * Utility Macros
 * ==============================================================================
 */

/* Compiler-specific attributes */
#ifdef _MSC_VER
    #define EC_INLINE __forceinline
    #define EC_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    #define EC_INLINE static inline __attribute__((always_inline))
    #define EC_NOINLINE __attribute__((noinline))
#else
    #define EC_INLINE static inline
    #define EC_NOINLINE
#endif

/* Thread-local storage */
#ifdef _MSC_VER
    #define EC_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
    #define EC_THREAD_LOCAL __thread
#else
    #define EC_THREAD_LOCAL /* Not supported */
#endif

#ifdef __cplusplus
}
#endif

#endif /* EVENTCHAINS_PLATFORM_H */