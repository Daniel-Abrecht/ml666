#ifndef ML666_REFCOUNT_H
#define ML666_REFCOUNT_H

#ifdef ML666_REFCOUNT_DEBUG
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
pid_t gettid(void);
#ifndef ML666_REFCOUNT_PARANOID
#define ML666_REFCOUNT_PARANOID
#endif
#endif

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

// Note: it's absolutely idiotic that there is an __STDC_NO_ATOMICS__ macro instead of a __STDC_ATOMICS__ macro.
// Non conforming compilers per default fail to specify it. Please fire whoever thought a negative feature test macro was a good idea.
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>

struct ml666_refcount {
  atomic_int_fast64_t value;
};

/**
 * Increment the ml666_refcount
 */
static inline void ml666_refcount_increment(struct ml666_refcount* ml666_refcount){
#ifdef ML666_REFCOUNT_PARANOID
  // Even when incrementing the ml666_refcount as fast as possible on the best of todays cpus,
  // it should take years to overflow. It isn't worth worrying about this ever happening.
  int_fast64_t ml666_refcount_increment_result = atomic_fetch_add_explicit(&ml666_refcount->value, 1, memory_order_relaxed);
  assert(ml666_refcount_increment_result >= 0);
#else
  atomic_fetch_add_explicit(&ml666_refcount->value, 1, memory_order_relaxed);
#endif
}

/**
 * Decrement the ml666_refcount
 * \returns false if the reference count has hit 0, true otherwise
 */
static inline bool ml666_refcount_decrement(struct ml666_refcount* ml666_refcount){
#ifdef ML666_REFCOUNT_PARANOID
  int_fast64_t ml666_refcount_decrement_result = atomic_fetch_sub_explicit(&ml666_refcount->value, 1, memory_order_acq_rel) - 1;
  assert(ml666_refcount_decrement_result >= 0);
  return ml666_refcount_decrement_result;
#else
  return atomic_fetch_sub_explicit(&ml666_refcount->value, 1, memory_order_acq_rel) - 1;
#endif
}

/**
 * Checks if the ml666_refcount is 1. This usually means we have the only existing reference.
 * \returns true if the ml666_refcount is 1, 0 otherwise.
 */
static inline bool ml666_refcount_is_last(struct ml666_refcount* ml666_refcount){
#ifdef ML666_REFCOUNT_PARANOID
  int_fast64_t ml666_refcount_is_last_result = atomic_load_explicit(&ml666_refcount->value, memory_order_acquire);
  assert(ml666_refcount_is_last_result >= 0);
  return ml666_refcount_is_last_result == 1;
#else
  return atomic_load_explicit(&ml666_refcount->value, memory_order_acquire) == 1;
#endif
}

/**
 * Checks if the ml666_refcount is 0. This means there are no references.
 * This may be useful when conditionally allocating datastructures, although such cases usually still need some additional locking.
 * \returns true if the ml666_refcount is 1, 0 otherwise.
 */
static inline bool ml666_refcount_is_zero(struct ml666_refcount* ml666_refcount){
#ifdef ML666_REFCOUNT_PARANOID
  int_fast64_t ml666_refcount_is_last_result = atomic_load_explicit(&ml666_refcount->value, memory_order_acquire);
  assert(ml666_refcount_is_last_result >= 0);
  return ml666_refcount_is_last_result == 0;
#else
  return atomic_load_explicit(&ml666_refcount->value, memory_order_acquire) == 0;
#endif
}


#else
#error "This file currently absolutely needs C11 atomic support. But feel free to add a fallback here."
#endif

#ifdef ML666_REFCOUNT_DEBUG
#define ml666_refcount_increment(R) \
  do { \
    struct ml666_refcount* _ml666_refcount = (R); \
    fprintf(stderr, "%ld> %s:%d: %s: ml666_refcount_increment(%p: %s)\n", (long)gettid(), __FILE__, __LINE__, __func__, (void*)_ml666_refcount, #R); \
    ml666_refcount_increment(_ml666_refcount); \
  } while(0)

#define ml666_refcount_decrement(R) \
  ( \
    fprintf(stderr, "%ld> %s:%d: %s: ml666_refcount_decrement(%p: %s) = " , (long)gettid(), __FILE__, __LINE__, __func__, (void*)(R), #R), \
    ml666_refcount_decrement((R)) \
     ? (fprintf(stderr, "true\n" ), true ) \
     : (fprintf(stderr, "false\n"), false) \
  )

#define ml666_refcount_is_last(R) \
  ( \
    fprintf(stderr, "%ld> %s:%d: %s: ml666_refcount_is_last(%p: %s) = " , (long)gettid(), __FILE__, __LINE__, __func__, (void*)(R), #R), \
    ml666_refcount_is_last((R)) \
     ? (fprintf(stderr, "true\n" ), true ) \
     : (fprintf(stderr, "false\n"), false) \
  )

#define ml666_refcount_is_zero(R) \
  ( \
    fprintf(stderr, "%ld> %s:%d: %s: ml666_refcount_is_zero(%p: %s) = " , (long)gettid(), __FILE__, __LINE__, __func__, (void*)(R), #R), \
    ml666_refcount_is_zero((R)) \
     ? (fprintf(stderr, "true\n" ), true ) \
     : (fprintf(stderr, "false\n"), false) \
  )

#endif

#endif
