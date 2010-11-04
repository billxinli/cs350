#include "opt-A2.h"

#if OPT_A2

#include <kern/unistd.h>
#include <types.h>
#include <thread.h>
#include <curthread.h>
#include <lib.h>
#include <kern/errno.h>
#include <child_table.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <addrspace.h>
#include <filetable.h>

#ifdef _ERRNO_H_
fkjldslfjdsklfjskldjf;
#endif

pid_t sys_fork(struct trapframe *tf) {
    int spl = splhigh();
    char *child_name = kmalloc(sizeof(char) * (strlen(curthread->t_name)+9));
    if (child_name == NULL) {
        ///errno won't work, so I've commented it out until it's fixed
        //errno = ENOMEM;
        return -1;
    }
    child_name = strcpy(child_name, curthread->t_name);
    struct child_table *new_child = kmalloc(sizeof(struct child_table));
    if (new_child == NULL) {
        ///errno won't work, so I've commented it out until it's fixed
        //errno = ENOMEM;
        return -1;
    }
    struct thread *child = NULL;
    
    int result = thread_fork(strcat(child_name, "'s child"), tf, 0, md_forkentry, &child);
    if (result != 0) {
        kfree(new_child);
        ///errno won't work, so I've commented it out until it's fixed
        //errno = result;
        return -1;
    }
    /*
    t_stack is technically supposed to be private, but since this is the only time we'll
    be using t_stack outside of thread functions, I've decided to just access the
    variable directly instead of making a new function that will only ever be used here
    */
    int i;
    for (i = 0; i < STACK_SIZE; i++) {
        child->t_stack[i] = curthread->t_stack[i];
    }
    
    child->parent = curthread;
    //add new process to list of children
    new_child->pid = child->pid;
    new_child->finished = 0;
    new_child->exit_code = -2; //not really needed
    new_child->next = curthread->children;
    curthread->children = new_child;
    
    //we don't need to copy t_sleepaddr because the thread can't be sleeping if it's calling fork()
    //we also don't need to copy t_cwd since it's already been coppied when thread_fork() was called
    as_copy(curthread->t_vmspace, &child->t_vmspace); //copy the data to the child process's new address space
    //now copy the file table
    int j;
    for (j = 0; j < ft_size(curthread->ft); j++) {
        ft_add(child->ft, ft_get(curthread->ft, j));
    }
    
    
    int retval = child->pid;
    
    splx(spl);
    
    return retval; //the parent thread returns this.
}

#endif
