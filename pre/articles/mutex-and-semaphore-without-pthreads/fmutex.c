#define _DEFAULT_SOURCE (1)

#include <fmutex.h>

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>

// Allocates memory for and create
// Default state is unlocked
Fmutex *fmutex_create(void) {
    void *memory;
    Fmutex *mutex;

    memory = malloc(fmutex_advise());
    if (memory == NULL) {
        return NULL;
    }

    if(fmutex_init(&mutex, memory) != 0) {
        free(memory);
        memory = NULL;
        return NULL;
    }

    return mutex;
}

// Advise how much memory is needed
size_t fmutex_advise(void) {
    return 1 * sizeof(Fmutex);
}

// Advise for many
size_t fmutex_advisev(size_t num_mutexes) {
    return num_mutexes * (1 * sizeof(Fmutex));
}

// Initialize
// Default state is unlocked
int fmutex_init(Fmutex **dest, void *memory) {
    if(dest == NULL || memory == NULL) {
        return EINVAL;
    }

    memset(memory, 0, sizeof(Fmutex));
    *dest = memory;
    (*dest)->locked = false;

    return 0;
}

// Initialize for many
int fmutex_initv(size_t num_mutexes, Fmutex *dest[], void *memory) {
    if(num_mutexes == 0 || dest == NULL || memory == NULL) {
        return EINVAL;
    }

    memset(memory, 0, num_mutexes * sizeof(Fmutex));
    *dest = memory;
    for(size_t i = 0; i < num_mutexes; i++) {
        dest[i]->locked = false;
    }

    return 0;
}

// Deinitialize
void fmutex_deinit(Fmutex *mutex) {
    if(mutex == NULL) {
        return;
    }

    mutex->locked = false;

    return;
}

/*
 * Destroys a Fmutex that was allocated by fmutex_create.
 * Please, only use with memory allocated by fmutex_create!
 */
void fmutex_destroy(Fmutex *mutex) {
    if(mutex == NULL) {
        return;
    }

    fmutex_deinit(mutex);
    free(mutex);

    return;
}

// lock mutex
// forces calling thread to wait if currently locked
// can lead to deadlock
int fmutex_lock(Fmutex *mutex) {
    long res;
    uint32_t expected = false;

    if(mutex == NULL) {
        return EINVAL;
    }

    while(!(__atomic_compare_exchange_n(&(mutex->locked), &expected, true, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))) {
        // want to perform FUTEX_WAIT_PRIVATE syscall
        // Successful result i.e. result in which sem->counter != 0 can be signaled in two ways
        // First, syscall itself returns 0 if waited then awaken
        // Two, syscall failed with -1 as sem->counter was not 0, errno is EAGAIN

        errno = 0;
        res = syscall(SYS_futex, &(mutex->locked), FUTEX_WAIT_PRIVATE, true, NULL);
        if(res == 0 || errno == EAGAIN) {
            // proceed
        } else {
            printf("error with FUTEX_WAIT in %s = %d\n", __func__, errno);
            return errno;
        }
        errno = 0;
    }

    return 0;
}

// unlock mutex
// fails if mutex is already unlocked
int fmutex_unlock(Fmutex *mutex) {
    long res;
    uint32_t expected = true;
    bool success;
    if(mutex == NULL) {
        return EINVAL;
    }

    success = __atomic_compare_exchange_n(&(mutex->locked), &expected, false, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
    if(success) {
        errno = 0;
        res = syscall(SYS_futex, &(mutex->locked), FUTEX_WAKE_PRIVATE, 1);
        if(res < 0) {
            printf("error with FUTEX_WAKE in %s = %d\n", __func__, errno);
            return res;
        }
    } else {
        // expected is update with the current value on failure
        assert(expected == false);
    }

    return 0;
}
