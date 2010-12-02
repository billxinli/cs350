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

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
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

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
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

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
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
	
#if OPT_A1
	lock->owner = NULL;
	lock->acquired = 0;
#endif
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);
	
#if OPT_A1
    assert(lock->acquired == 0); //lock should be released before it is destroyed
#endif
	
	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
#if OPT_A1
    assert(lock != NULL);
    int spl = splhigh(); //disable interrupts
	while (lock->acquired) {
	    assert(lock->owner != curthread); //if the thread tries to aquire the same lock twice without releasing first, throw error and quit
	    thread_sleep(lock);
	}
	lock->acquired = 1;
	lock->owner = curthread;
	splx(spl); //re-enable interrupts
#else
    (void)lock;
#endif
}

void
lock_release(struct lock *lock)
{
#if OPT_A1
    int spl = splhigh();
    assert(lock != NULL);
	assert(lock->owner == curthread);
	assert(lock->acquired != 0); //cannot release a lock that has already been released
	lock->acquired = 0;
	thread_wakeup(lock);
	splx(spl);
#else
    (void)lock;
#endif
}

int
lock_do_i_hold(struct lock *lock)
{
#if OPT_A1
    assert(lock != NULL);
	return (lock->acquired && lock->owner == curthread);
#else
    (void)lock;
    return 1;
#endif
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
#if OPT_A1
    cv->first = NULL;
    cv->last = NULL;
#endif
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);
#if OPT_A1
    assert(cv->first == NULL); //CV should not be destroyed if there are still threads waiting on the CV
#endif
	
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
#if OPT_A1
	int spl = splhigh(); //disable interrupts
	struct wait_list *sleeper;
	sleeper = kmalloc(sizeof(struct wait_list));
	if (sleeper == NULL) {
	    panic("Out of memory!");
	}
	sleeper->signal = 0;
	sleeper->lock = lock;
	sleeper->next = NULL;
	//add new sleeper to list of waiting threads
	if (cv->first == NULL) {
	    cv->first = sleeper;
	} else {
	    cv->last->next = sleeper;
	}
	cv->last = sleeper;
	
	lock_release(lock); //release the lock
	
	while (sleeper->signal == 0) {
	    thread_sleep(cv);
	}
	
	kfree(sleeper); //safe, since when cv_signal sets the signal value to 1, cv->first no longer points to it
	lock_acquire(lock); //re-aquire the lock
	splx(spl); //re-enable interrupts
#else
    (void)cv;
    (void)lock;
#endif
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
#if OPT_A1
	int spl = splhigh(); //disable interrupts
	struct wait_list *p = cv->first;
	struct wait_list *prev;
	while (p != NULL) {
	    if (p->lock == lock) {
	        p->signal = 1; //set the next waiting thread to be woken up
	        if (p == cv->first) {
	            cv->first = cv->first->next; //move the rest of the threads up in line
	        } else {
	            prev->next = p->next; //remove this link from the list
	        }
	        break;
	    } else {
	        prev = p;
	        p = p->next;
	    }
	}
	thread_wakeup(cv);
	splx(spl); //re-enable interrupts
#else
    (void)cv;
    (void)lock;
#endif
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
#if OPT_A1
	int spl = splhigh(); //disable interrupts
	struct wait_list *p = cv->first;
	struct wait_list *prev;
	while (p != NULL) {
	    if (p->lock == lock) {
	        p->signal = 1; //set the next waiting thread to be woken up
	        if (p == cv->first) {
	            cv->first = cv->first->next; //move the rest of the threads up in line
	        } else {
	            prev->next = p->next; //remove this link from the list
	        }
	    }
	    prev = p;
	    p = p->next;
	}
	thread_wakeup(cv);
	splx(spl); //re-enable interrupts
#else
	(void)cv;
	(void)lock;
#endif
}
