// lock mutex
// forces calling thread to wait if currently locked
// can lead to deadlock
int fmutex_lock(Fmutex *mutex) {
    long res;
    uint32_t expected = false;

    if(mutex == NULL) {
        return EINVAL;
    }

    while(!(atomic_compare_exchange_strong_explicit(&(mutex->locked),
            &expected, true,
            memory_order_acq_rel,
            memory_order_acquire))) {
        // want to perform FUTEX_WAIT_PRIVATE syscall
        // Successful result (sem->counter != 0) can be signaled in two ways
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
