int fsemaphore_post(Fsemaphore *sem) {
    bool mutex_locked = false;
    long res;

    res = fmutex_lock(sem->mutex);
    if(res != 0) {
        goto fsemaphore_post_error;
    }
    mutex_locked = true;
    if(__atomic_load_n(&(sem->counter), __ATOMIC_ACQUIRE) == sem->max) {
        res = EDEADLK;
        goto fsemaphore_post_error;
    } else {
        __atomic_fetch_add(&(sem->counter), 1, __ATOMIC_ACQ_REL);

        errno = 0;
        res = syscall(SYS_futex, &(sem->counter), FUTEX_WAKE_PRIVATE, 1);
        if(res < 0) {
            goto fsemaphore_post_error;
        }
        res = 0;

        res = fmutex_unlock(sem->mutex);
        if(res != 0) {
            goto fsemaphore_post_error;
        }
        mutex_locked = false;
    }

fsemaphore_post_error:
    if(mutex_locked) {
        fmutex_unlock(sem->mutex);
        mutex_locked = false;
    }
    if(res != 0) {
        printf("error with FUTEX_WAKE in %s = %d\n", __func__, errno);
    }
    return 0;
}
