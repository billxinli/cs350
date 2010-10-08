/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>
#include "opt-A1.h"

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *sem_create(const char *namearg, int initial_count) {
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void sem_destroy(struct semaphore *sem) {
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem) == 0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void P(struct semaphore *sem) {
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt == 0);

	spl = splhigh();
	while (sem->count == 0) {
		thread_sleep(sem);
	}
	assert(sem->count > 0);
	sem->count--;
	splx(spl);
}

void V(struct semaphore *sem) {
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count > 0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *lock_create(const char *name) {
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	// add stuff here as needed

#if OPT_A1
	lock->thread = NULL;
#endif

	return lock;
}

void lock_destroy(struct lock *lock) {
	assert(lock != NULL);

	// add stuff here as needed

	kfree(lock->name);
	kfree(lock);
}

void lock_acquire(struct lock *lock) {

#if OPT_A1
	int spl;
	assert(lock != NULL);
	assert(in_interrupt == 0);
	//turn off interrupt
	spl = splhigh();
	while (lock->thread != NULL) {
		thread_sleep(lock);
	}
	assert(lock->thread == NULL);
	//set the current thread as the thread that is holding the lock
	lock->thread = curthread;
	splx(spl);
#else

	// Write this
	(void)lock;		// suppress warning until code gets written

#endif
}

void lock_release(struct lock *lock) {

#if OPT_A1
	int spl;
	assert(lock != NULL);
	assert(in_interrupt == 0);

	//turn off interrupt
	spl = splhigh();
	//make sure that you actually hold the lock
	assert(lock_do_i_hold(lock));
	//release the lock, the second assert shouldn't happen
	lock->thread = NULL;
	assert(lock->thread == NULL);
	//wake up all the awaiting locks
	thread_wakeup(lock);
	splx(spl);
#else

	// Write this
	(void)lock;		// suppress warning until code gets written

#endif
}

int lock_do_i_hold(struct lock *lock) {
#if OPT_A1
	int spl;
	int status;
	assert(lock != NULL);
	assert(in_interrupt == 0);
	//turn off interrupt
	spl = splhigh();
	//are we the owner of this lock?
	status = (curthread == lock->thread);
	splx(spl);
	//return the int
	return status;
#else
	// Write this
	(void)lock;		// suppress warning until code gets written
	return 1;		// dummy until code gets written

#endif

}

////////////////////////////////////////////////////////////
//
// CV

struct cv *cv_create(const char *name) {
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name == NULL) {
		kfree(cv);
		return NULL;
	}
	// add stuff here as needed

	return cv;
}

void cv_destroy(struct cv *cv) {
	assert(cv != NULL);

	// add stuff here as needed

	kfree(cv->name);
	kfree(cv);
}

void cv_wait(struct cv *cv, struct lock *lock) {
#if OPT_A1
	int spl;
	assert(lock != NULL);
	assert(cv != NULL);
	//turn off interrupt
	spl = splhigh();
	//otherwise you can't release the lock
	assert(lock_do_i_hold(lock));
	//release the lock
	lock_release(lock);
	//put this thread to sleep on cv
	thread_sleep(cv);
	//after waking up try to acquire the locka gain
	lock_acquire(lock);
	splx(spl);
#else
	// Write this
	(void)cv;		// suppress warning until code gets written
	(void)lock;		// suppress warning until code gets written

#endif
}

void cv_signal(struct cv *cv, struct lock *lock) {
#if OPT_A1
	int spl;
	assert(lock != NULL);
	assert(cv != NULL);
	//turn off interrupt
	spl = splhigh();
	//wake up 1 thread that is sleeping on cv
	thread_wakeup_one(cv);
	splx(spl);
#else
	// Write this
	(void)cv;		// suppress warning until code gets written
	(void)lock;		// suppress warning until code gets written

#endif
}

void cv_broadcast(struct cv *cv, struct lock *lock) {
#if OPT_A1
	int spl;
	assert(lock != NULL);
	assert(cv != NULL);
	//turn off interrupt
	spl = splhigh();
	//wake up all the bloody threads
	thread_wakeup(cv);
	splx(spl);
#else
	// Write this
	(void)cv;		// suppress warning until code gets written
	(void)lock;		// suppress warning until code gets written

#endif
}
