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
        if(__atomic_load_n(&(sem->counter), __ATOMIC_ACQUIRE) > 0) {
            __atomic_fetch_sub(&(sem->counter), 1, __ATOMIC_ACQ_REL);
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
