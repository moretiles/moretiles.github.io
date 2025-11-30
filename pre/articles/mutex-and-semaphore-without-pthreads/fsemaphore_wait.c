int fsemaphore_wait(Fsemaphore *sem) {
    long res;
    bool mutex_locked = false;
    bool decremented = false;
    if(sem == NULL) {
        return EINVAL;
    }

    while(!decremented) {
        res = fmutex_lock(sem->mutex);
        if(res != 0) {
            goto fsemaphore_wait_error;
        }
        mutex_locked = true;
        if(atomic_load_explicit(&(sem->counter), memory_order_acquire) > 0) {
            atomic_fetch_sub_explicit(&(sem->counter), 1, memory_order_release);
            decremented = true;
        } else {
            res = fmutex_unlock(sem->mutex);
            if(res != 0) {
                goto fsemaphore_wait_error;
            }
            mutex_locked = false;

            // want to perform FUTEX_WAIT_PRIVATE syscall
            // Successful result (sem->counter != 0) can be signaled in two ways
            // First, syscall itself returns 0 if waited then awaken
            // Two, syscall failed with -1 as sem->counter was not 0, errno is EAGAIN

            errno = 0;
            res = syscall(SYS_futex, &(sem->counter), FUTEX_WAIT_PRIVATE, 0, NULL);
            if(res == 0 || errno == EAGAIN) {
                // proceed
            } else {
                res = errno;
                goto fsemaphore_wait_error;
            }
            errno = 0;
        }
    }

fsemaphore_wait_error:
    if(mutex_locked) {
        fmutex_unlock(sem->mutex);
        mutex_locked = false;
    }
    if(res != 0) {
        printf("error with FUTEX_WAKE in %s = %ld\n", __func__, res);
    }
    return res;
}
