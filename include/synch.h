#include <curthread.h>
#include "opt-A1.h"
#include "opt-A3.h"

/*
 * Header file for synchronization primitives.
 */

#ifndef _SYNCH_H_
#define _SYNCH_H_

/*
 * Dijkstra-style semaphore.
 * Operations:
 *     P (proberen): decrement count. If the count is 0, block until
 *                   the count is 1 again before decrementing.
 *     V (verhogen): increment count.
 * 
 * Both operations are atomic.
 *
 * The name field is for easier debugging. A copy of the name is made
 * internally.
 */

struct semaphore {
	char *name;
	volatile int count;
};

struct semaphore *sem_create(const char *name, int initial_count);
void              P(struct semaphore *);
void              V(struct semaphore *);
void              sem_destroy(struct semaphore *);


/*
 * Simple lock for mutual exclusion.
 * Operations:
 *    lock_acquire - Get the lock. Only one thread can hold the lock at the
 *                   same time.
 *    lock_release - Free the lock. Only the thread holding the lock may do
 *                   this.
 *    lock_do_i_hold - Return true if the current thread holds the lock; 
 *                   false otherwise.
 *
 * These operations must be atomic. You get to write them.
 *
 * When the lock is created, no thread should be holding it. Likewise,
 * when the lock is destroyed, no thread should be holding it.
 *
 * The name field is for easier debugging. A copy of the name is made
 * internally.
 */

struct lock {
	char *name;
#if OPT_A1
	struct thread *owner; //the thread that aquires the lock
	volatile int acquired; //weather the lock has been acquired or not
#endif
};

struct lock *lock_create(const char *name);
void         lock_acquire(struct lock *);
void         lock_release(struct lock *);
int          lock_do_i_hold(struct lock *);
void         lock_destroy(struct lock *);
#if OPT_A3
struct lock *lock_create_nokmalloc(const char *name);
#endif


/*
 * Condition variable.
 *
 * Note that the "variable" is a bit of a misnomer: a CV is normally used
 * to wait until a variable meets a particular condition, but there's no
 * actual variable, as such, in the CV.
 *
 * Operations:
 *    cv_wait      - Release the supplied lock, go to sleep, and, after
 *                   waking up again, re-acquire the lock.
 *    cv_signal    - Wake up one thread that's sleeping on this CV.
 *    cv_broadcast - Wake up all threads sleeping on this CV.
 *
 * For all three operations, the current thread must hold the lock passed 
 * in. Note that under normal circumstances the same lock should be used
 * on all operations with any particular CV.
 *
 * These operations must be atomic. You get to write them.
 *
 * These CVs are expected to support Mesa semantics, that is, no
 * guarantees are made about scheduling.
 *
 * The name field is for easier debugging. A copy of the name is made
 * internally.
 */

#if OPT_A1
//a list structure for use in my CV implementation
struct wait_list {
    volatile int signal; //1 if the thread should be woken up, 0 otherwise
    struct lock *lock;
    struct wait_list *next;
};
#endif
 
struct cv {
	char *name;
#if OPT_A1
    struct wait_list *first; //next thread to be woken up
    struct wait_list *last; //most recently added thread
#endif
};

struct cv *cv_create(const char *name);
void       cv_wait(struct cv *cv, struct lock *lock);
void       cv_signal(struct cv *cv, struct lock *lock);
void       cv_broadcast(struct cv *cv, struct lock *lock);
void       cv_destroy(struct cv *);

#endif /* _SYNCH_H_ */
