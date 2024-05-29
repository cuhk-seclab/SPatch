void queued_read_lock_slowpath(struct qrwlock *lock, u32 cnts)
{
	/*
	 * Readers come here when they cannot get the lock without waiting
	 */
	if (unlikely(in_interrupt())) {
		/*
		 * Readers in interrupt context will get the lock immediately
		 * if the writer is just waiting (not holding the lock yet).
		 * The rspin_until_writer_unlock() function returns immediately
		 * in this case. Otherwise, they will spin (with ACQUIRE
		 * semantics) until the lock is available without waiting in
		 * the queue.
		 */
		rspin_until_writer_unlock(lock, cnts);
		return;
	}
	atomic_sub(_QR_BIAS, &lock->cnts);

	/*
	 * Put the reader into the wait queue
	 */
	arch_spin_lock(&lock->lock);

	/*
	 * The ACQUIRE semantics of the following spinning code ensure
	 * that accesses can't leak upwards out of our subsequent critical
	 * section in the case that the lock is currently held for write.
	 */
	cnts = atomic_add_return_acquire(_QR_BIAS, &lock->cnts) - _QR_BIAS;
	rspin_until_writer_unlock(lock, cnts);

	/*
	 * Signal the next one in queue to become queue head
	 */
	arch_spin_unlock(&lock->lock);
}
