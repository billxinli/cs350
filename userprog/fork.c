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

pid_t sys_fork(struct trapframe *tf) {
    int spl = splhigh();
    char *child_name = kmalloc(sizeof(char) * (strlen(curthread->t_name)+9));
    if (child_name == NULL) {
        //error
        splx(spl);
        return ENOMEM;
    }
    child_name = strcpy(child_name, curthread->t_name);
    struct child_table *new_child = kmalloc(sizeof(struct child_table));
    if (new_child == NULL) {
        //error
        splx(spl);
        return ENOMEM;
    }
    struct thread *child = NULL;

    void (*func_pt)(void *, unsigned long) = &md_forkentry;
    
    int result = thread_fork(strcat(child_name, "'s child"), tf, 0, func_pt, &child);
   
    if (result != 0) {
        kfree(new_child);
        //ERROR
        splx(spl);
        return result;
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
    int err = as_copy(curthread->t_vmspace, &child->t_vmspace); //copy the data to the child process's new address space
    if (err != 0) {
        DEBUG(DB_THREADS, "Not enough memory to copy address space in fork. Closing child...\n");
        child->t_vmspace = NULL;
        child->parent = NULL; //to prevent thread_destroy from freeing a non-existant pid
        md_initpcb(&child->t_pcb, child->t_stack, 0, 0, &thread_exit); //set new thread to delete itself
        splx(spl);
        return err;
    }
    //now copy the file table
    int j;
    for (j = 0; j < ft_size(curthread->ft); j++) {
        if (ft_add(child->ft, ft_get(curthread->ft, j)) == -1) {
            DEBUG(DB_THREADS, "Not enough memory to copy file table in fork. Closing child...\n");
            child->parent = NULL; //to prevent thread_destroy from freeing a non-existant pid
            md_initpcb(&child->t_pcb, child->t_stack, 0, 0, &thread_exit); //set new thread to delete itself
            splx(spl);
            return ENOMEM;
        }
    }
    
    
    int retval = child->pid;
    splx(spl);
    return retval; //the parent thread returns this.
}

#endif
