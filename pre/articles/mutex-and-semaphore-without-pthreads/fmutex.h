/*
 * fmutex.h -- Mutex data structure implemented using futex.
 * Relies on Linux system calls.
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef DERT_FMUTEX
#define DERT_FMUTEX 1
typedef struct fmutex {
    // Linux FUTEX syscall requires a uint32_t*
    // So, use this uint32_t as a boolean
    uint32_t locked;
} Fmutex;

// Allocates memory for and create
// Default state is unlocked
Fmutex *fmutex_create(void);

// Advise how much memory is needed
size_t fmutex_advise(void);

// Advise for many
size_t fmutex_advisev(size_t num_mutexes);

// Initialize
// Default state is unlocked
int fmutex_init(Fmutex **dest, void *memory);

// Initialize for many
int fmutex_initv(size_t num_mutexes, Fmutex *dest[], void *memory);

// Deinitialize
void fmutex_deinit(Fmutex *mutex);

/*
 * Destroys a Fmutex that was allocated by fmutex_create.
 * Please, only use with memory allocated by fmutex_create!
 */
void fmutex_destroy(Fmutex *mutex);

// lock mutex
// forces calling thread to wait if currently locked
// can lead to deadlock
int fmutex_lock(Fmutex *mutex);

// unlock mutex
// fails if mutex is already unlocked
int fmutex_unlock(Fmutex *mutex);
#endif
