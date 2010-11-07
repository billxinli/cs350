/*
Name
_exit - terminate process
Library
Standard C Library (libc, -lc)
Synopsis
#include <unistd.h>

void
_exit(int exitcode);
Description
Cause the current process to exit. The exit code exitcode is reported back to other process(es) via the waitpid() call. The process id of the exiting process should not be reused until all processes interested in collecting the exit code with waitpid have done so. (What "interested" means is intentionally left vague; you should design this.)
Return Values
_exit does not return. 
*/
#include "opt-A2.h"

#if OPT_A2
#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <thread.h>
#include <curthread.h>
#include <lib.h>

void sys__exit(int exitcode){
    /*
    The code for freeing resources used by a process are found in thread.c
    since we need to be able to free these resources on processes that exit
    abnormally (crash without calling _exit())
    */
    DEBUG(DB_THREADS, "Thread `%s` calling _exit()\n", curthread->t_name);
	curthread->exit_status = exitcode;
	thread_exit();
}




#endif /* OPT_A2 */
