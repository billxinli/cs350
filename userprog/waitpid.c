#include "opt-A2.h"

#if OPT_A2

#include <machine/spl.h>
#include <kern/unistd.h>
#include <pid.h>
#include <child_table.h>
#include <thread.h>
#include <curthread.h>
#include <kern/errno.h>
#include <lib.h>

int sys_waitpid(pid_t PID, int *status, int options) {
    int spl = splhigh();
    if (options != 0) {
        ///errno won't work, so I've commented it out until it's fixed
        //errno = EINVAL;
        splx(spl);
        return -1;
    }
    struct child_table *child = NULL;
    struct child_table *p;
    for (p = curthread->children; p != NULL; p = p->next) {
        if (p->pid == PID) {
            child = p;
            break;
        }
    }
    if (child == NULL) { //error: pid not in use or is not the pid of a child process
        ///errno won't work, so I've commented it out until it's fixed
        //errno = ESRCH;
        splx(spl);
        return -1;
    }
    
    while (child->finished == 0) {
        thread_sleep((void *) PID);
    }
    
    *status = child->exit_code;
    
    //now, remove the child from children list since it has exited and it's PID is no longer needed
    if (curthread->children->pid == PID) {
        struct child_table *temp = curthread->children;
        curthread->children = curthread->children->next;
        kfree(temp);
    } else {
        for (p = curthread->children; p != NULL; p = p->next) {
            if (p->next->pid == PID) {
                struct child_table *temp = p->next;
                p->next = p->next->next;
                kfree(temp);
                break;
            }
        }
    }
    
    pid_parent_done(PID);
    splx(spl);
    
    return PID;
}

#endif