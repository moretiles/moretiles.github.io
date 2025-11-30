int fmutex_unlock(Fmutex *mutex) {
    long res;
    uint32_t expected = true;
    bool success;
    if(mutex == NULL) {
        return EINVAL;
    }

    success = __atomic_compare_exchange_n(&(mutex->locked),
                                          &expected, false, false,
                                          __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
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
