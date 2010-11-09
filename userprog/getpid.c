/*
Note: the code for getting new pids and reclaiming pids once processes terminate
is found in /threads/pid.c. This is just the function to get the pid of the
currently executing thread
*/

#include "opt-A2.h"
#if OPT_A2

#include <types.h>
#include <thread.h>
#include <syscall.h>
#include <curthread.h>

pid_t sys_getpid(void) {
    return curthread->pid;
}

#endif
