/*
 * fsemaphore.h -- Semaphore data structure implemented using futex.
 * Relies on Linux system calls.
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <fmutex.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef DERT_FSEMAPHORE
#define DERT_FSEMAPHORE 1
typedef struct fsemaphore {
    Fmutex *mutex;
    _Atomic uint64_t counter;
    uint64_t max;
} Fsemaphore;

// Allocates memory for and create
// Default state is unlocked
Fsemaphore *fsemaphore_create(uint64_t val, uint64_t max);

// Advise how much memory is needed
size_t fsemaphore_advise(uint64_t val, uint64_t max);

// Advise for many
size_t fsemaphore_advisev(size_t num_sems, uint64_t val, uint64_t max);

// Initialize
// Default state is unlocked
int fsemaphore_init(Fsemaphore **dest, void *memory, uint64_t val, uint64_t max);

// Initialize for many
int fsemaphore_initv(size_t num_sems, Fsemaphore *dest[], void *memory, uint64_t val, uint64_t max);

// Deinitialize
void fsemaphore_deinit(Fsemaphore *sem);

/*
 * Destroys a Fsemaphore that was allocated by fsemaphore_create.
 * Please, only use with memory allocated by fsemaphore_create!
 */
void fsemaphore_destroy(Fsemaphore *sem);

// Set count for sem to 0
int fsemaphore_exhaust(Fsemaphore *sem);

// Decreate count for sem by decrement if count is greater than 0
// Else, block until available
int fsemaphore_wait(Fsemaphore *sem);

// Increment count for sem by increment
int fsemaphore_post(Fsemaphore *sem);

// Set count for sem to its max
int fsemaphore_reset(Fsemaphore *sem);
#endif
