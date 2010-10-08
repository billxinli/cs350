/*
 * catmouse.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 * 26-11-2007: KMS : Modified to use cat_eat and mouse_eat
 * 21-04-2009: KMS : modified to use cat_sleep and mouse_sleep
 * 21-04-2009: KMS : added sem_destroy of CatMouseWait
 *
 */

/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>

/*
 * 
 * cat,mouse,bowl simulation functions defined in bowls.c
 *
 * For Assignment 1, you should use these functions to make your cats and mice eat from the bowls.
 * 
 * You may *not* modify these functions in any way.
 * They are implemented in a separate file (bowls.c) to remind you that you should not change them.
 *
 * For information about the behaviour and return values of these functions, see bowls.c
 *
 */

/* this must be called before any calls to cat_eat or mouse_eat */
extern int initialize_bowls(unsigned int bowlcount);

extern void cat_eat(unsigned int bowlnumber);
extern void mouse_eat(unsigned int bowlnumber);
extern void cat_sleep(void);
extern void mouse_sleep(void);

/*
 *
 * Problem parameters
 *
 * Values for these parameters are set by the main driver function, catmouse(), based on the problem parameters that are passed in from the kernel menu command or kernel command line.
 *
 * Once they have been set, you probably shouldn't be changing them.
 *
 *
 */
int NumBowls;			// number of food bowls
int NumCats;			// number of cats
int NumMice;			// number of mice
int NumLoops;			// number of times each cat and mouse should eat

/*
 * Once the main driver function (catmouse()) has created the cat and mouse simulation threads, it uses this semaphore to block until all of the cat and mouse simulations are finished.
 */

struct semaphore *CatMouseWait;
struct semaphore *BowlThatIsEmpty;
struct lock *BowlSelectLock;
struct lock *CounterLock;
struct cv *ExclusiveCat;
struct cv *ExclusiveMouse;
volatile int numcat=0;
volatile int nummouse=0;
volatile char *bowls;
/*
 * 
 * Function Definitions
 * 
 */

/*
 * sync_var_init()
 *
 * Arguments:
 *      nothing.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This is called by the mouse and cat sim to init the null sync variables.
 */

static void sync_var_init() {

	if (ExclusiveCat == NULL) {
		ExclusiveCat = cv_create("ExclusiveCat");
		assert(ExclusiveCat != NULL);
	}

	if (ExclusiveMouse == NULL) {
		ExclusiveMouse = cv_create("ExclusiveMouse");
		assert(ExclusiveMouse != NULL);
	}

	if (BowlSelectLock == NULL) {
		BowlSelectLock = lock_create("BowlSelectLock");
		assert(BowlSelectLock != NULL);
	}

	if (CounterLock == NULL) {
		CounterLock = lock_create("CounterLock");
		assert(CounterLock != NULL);
	}

	if (BowlThatIsEmpty == NULL) {
		BowlThatIsEmpty = sem_create("BowlThatIsEmpty", NumBowls);
		assert(BowlThatIsEmpty != NULL);
	}

	if (bowls == NULL) {
		bowls = kmalloc(NumBowls * sizeof(char));
		assert(bowls != NULL);
	}

}
/*
 * cat_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds cat identifier from 0 to NumCats-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Each cat simulation thread runs this function.
 *
 *      You need to finish implementing this function using the OS161 synchronization primitives as indicated in the assignment description
 */

static void cat_simulation(void *unusedpointer, unsigned long catnumber) {
	sync_var_init();

	int i, bowl;
	/* avoid unused variable warnings. */
	(void)unusedpointer;
	(void)catnumber;
	for (i = 0; i < NumLoops; i++) {
		//cat_sleep();

		lock_acquire(BowlSelectLock);

		while (nummouse > 0) {
			cv_wait(ExclusiveCat, BowlSelectLock);
		}
		P(BowlThatIsEmpty);

		while (1) {
			bowl = ((unsigned int)random() % NumBowls) + 1;
			if (bowls[bowl - 1] != 'o') {
				bowls[bowl - 1] = 'o';
				break;
			}
		}
		lock_release(BowlSelectLock);

		lock_acquire(CounterLock);
		{
			numcat++;
		}
		lock_release(CounterLock);

		cat_eat(bowl);

		lock_acquire(CounterLock);
		{
			numcat--;
			bowls[bowl - 1] = '-';
			if (numcat == 0) {
				cv_broadcast(ExclusiveMouse, BowlSelectLock);
			}

		}
		lock_release(CounterLock);

		V(BowlThatIsEmpty);

	}

	/* indicate that this cat simulation is finished */
	V(CatMouseWait);
}

/*
 * mouse_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds mouse identifier from 0 to NumMice-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      each mouse simulation thread runs this function
 *
 *      You need to finish implementing this function using 
 *      the OS161 synchronization primitives as indicated
 *      in the assignment description
 *
 */

static void mouse_simulation(void *unusedpointer, unsigned long mousenumber) {

	sync_var_init();

	int i, bowl;
	/* Avoid unused variable warnings. */
	(void)unusedpointer;
	(void)mousenumber;

	for (i = 0; i < NumLoops; i++) {
		//mouse_sleep();
		lock_acquire(BowlSelectLock);

		while (numcat > 0) {
			cv_wait(ExclusiveMouse, BowlSelectLock);
		}

		P(BowlThatIsEmpty);

		while (1) {
			bowl = ((unsigned int)random() % NumBowls) + 1;
			if (bowls[bowl - 1] != 'o') {
				bowls[bowl - 1] = 'o';
				break;
			}
		}
		lock_release(BowlSelectLock);

		lock_acquire(CounterLock);
		{
			nummouse++;
		}
		lock_release(CounterLock);

		mouse_eat(bowl);

		lock_acquire(CounterLock);
		{

			nummouse--;
			bowls[bowl - 1] = '-';
			if (nummouse == 0) {
				cv_broadcast(ExclusiveCat, BowlSelectLock);

			}
		}
		lock_release(CounterLock);

		V(BowlThatIsEmpty);
	}
	/* indicate that this mouse is finished */
	V(CatMouseWait);
}

/*
 * catmouse()
 *
 * Arguments:
 *      int nargs: should be 5
 *      char ** args: args[1] = number of food bowls
 *                    args[2] = number of cats
 *                    args[3] = number of mice
 *                    args[4] = number of loops
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up cat_simulation() and
 *      mouse_simulation() threads.
 *      You may need to modify this function, e.g., to
 *      initialize synchronization primitives used
 *      by the cat and mouse threads.
 *      
 *      However, you should should ensure that this function
 *      continues to create the appropriate numbers of
 *      cat and mouse threads, to initialize the simulation,
 *      and to wait for all cats and mice to finish.
 */

int catmouse(int nargs, char **args) {
	int index, error;
	int i;
	/* check and process command line arguments */
	if (nargs != 5) {
		kprintf
		    ("Usage: <command> NUM_BOWLS NUM_CATS NUM_MICE NUM_LOOPS\n");
		return 1;	// return failure indication
	}

	/* check the problem parameters, and set the global variables */
	NumBowls = atoi(args[1]);
	if (NumBowls <= 0) {
		kprintf("catmouse: invalid number of bowls: %d\n", NumBowls);
		return 1;
	}
	NumCats = atoi(args[2]);
	if (NumCats < 0) {
		kprintf("catmouse: invalid number of cats: %d\n", NumCats);
		return 1;
	}
	NumMice = atoi(args[3]);
	if (NumMice < 0) {
		kprintf("catmouse: invalid number of mice: %d\n", NumMice);
		return 1;
	}
	NumLoops = atoi(args[4]);
	if (NumLoops <= 0) {
		kprintf("catmouse: invalid number of loops: %d\n", NumLoops);
		return 1;
	}
	kprintf
	    ("Using %d bowls, %d cats, and %d mice. Looping %d times.\n",
	     NumBowls, NumCats, NumMice, NumLoops);
	/* create the semaphore that is used to make the main thread
	   wait for all of the cats and mice to finish */
	CatMouseWait = sem_create("CatMouseWait", 0);
	if (CatMouseWait == NULL) {
		panic("catmouse: could not create semaphore\n");
	}

	/* 
	 * initialize the bowls
	 */
	if (initialize_bowls(NumBowls)) {
		panic("catmouse: error initializing bowls.\n");
	}

	/*
	 * Start NumCats cat_simulation() threads.
	 */
	for (index = 0; index < NumCats; index++) {
		error =
		    thread_fork("cat_simulation thread", NULL, index,
				cat_simulation, NULL);
		if (error) {
			panic("cat_simulation: thread_fork failed: %s\n",
			      strerror(error));
		}
	}

	/*
	 * Start NumMice mouse_simulation() threads.
	 */
	for (index = 0; index < NumMice; index++) {
		error =
		    thread_fork("mouse_simulation thread", NULL, index,
				mouse_simulation, NULL);
		if (error) {
			panic("mouse_simulation: thread_fork failed: %s\n",
			      strerror(error));
		}
	}

	/* wait for all of the cats and mice to finish before
	   terminating */
	for (i = 0; i < (NumCats + NumMice); i++) {
		P(CatMouseWait);
	}

	/* clean up the semaphore the we created */
	sem_destroy(CatMouseWait);

	return 0;
}

/*
 * End of catmouse.c
 */
