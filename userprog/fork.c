#if OPT_A2

#include <types.h>
#include <thread.h>
#include <curthread.h>
#include <lib.h>
#include <kern/errno.h>
#include <child_table.h>

pid_t sys_fork() {
    int spl = splhigh();
    char *child_name = kmalloc(sizeof(char) * (strlen(curthread->t_name)+9));
    if (child_name == NULL) {
        errno = ENOMEM;
        return -1;
    }
    child_name = strcpy(child_name, curthread->t_name);
    struct thread *child = kmalloc(sizeof(struct thread));
    if (thread == NULL) {
        errno = ENOMEM;
        return -1;
    }
    struct child_table *new_child = kmalloc(sizeof(struct child_table));
    if (new_child == NULL) {
        errno = ENOMEM;
        return -1;
    }
    child = thread_create(strcat(child_name, "'s child"));
    child->parent = curthread;
    //add new process to list of children
    new_child->pid = child->pid;
    new_child->finished = 0;
    new_child->exit_code = -2; //not really needed
    new_child->next = curthread->children;
    curthread->children = new_child;
    
    ///copy address space
    
    ///copy trap frame
    
    ///set return value in trap frame of child to approprite return value
    
    ///return in parent thread
    
    splx(spl);
}

#endif
