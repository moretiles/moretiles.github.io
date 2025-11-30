int fsemaphore_post(Fsemaphore *sem) {
    bool mutex_locked = false;
    long res;

    res = fmutex_lock(sem->mutex);
    if(res != 0) {
        goto fsemaphore_post_error;
    }
    mutex_locked = true;
    if(atomic_load_explicit(&(sem->counter), memory_order_acquire) == sem->max) {
        res = EDEADLK;
        goto fsemaphore_post_error;
    } else {
        atomic_fetch_add_explicit(&(sem->counter), 1, memory_order_release);

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
