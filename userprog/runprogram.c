/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>
#include "opt-A2.h"
#include "opt-A3.h"

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */

#if OPT_A3

int runprogram(char *progname, char ** args, unsigned long nargs) {
   
    vaddr_t entrypoint, stackptr;
    int result;

 
    /* We should be a new thread. */
    assert(curthread->t_vmspace == NULL);

    /* Create a new address space. */
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) {
       
        return ENOMEM;
    }

    /* Activate it. */
    as_activate(curthread->t_vmspace);

    assert(curthread->t_vmspace != NULL);

    /* Load the executable. */
    result = load_elf(progname, &entrypoint);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
    
        return result;
    }



    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
        return result;
    }

    /* calculate the size of the stack frame for the main() function */
    int stackFrameSize = 8; //minimum size for argc=0, argv[0]=NULL
    unsigned long i;
    for (i = 0; i < nargs; i++) {
        stackFrameSize += strlen(args[i]) + 1 + 4; //add the length of each argument, plus the space for the pointer
    }

    /* Decide the stackpointer address */
    stackptr -= stackFrameSize;
    //decrement the stackptr until it is divisible by 8
    for (; (stackptr % 8) > 0; stackptr--) {
    }

    /* copy the arguments to the proper location on the stack */
    int argumentStringLocation = (int) stackptr + 4 + ((nargs + 1) * 4); //begining of where strings will be stored
    copyout((void *) & nargs, (userptr_t) stackptr, (size_t) 4); //copy argc

    for (i = 0; i < nargs; i++) {
        copyout((void *) & argumentStringLocation, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy address of string into argv[i+1]
        copyoutstr(args[i], (userptr_t) argumentStringLocation, (size_t) strlen(args[i]), NULL); //copy the argument string
        argumentStringLocation += strlen(args[i]) + 1; //update the location for the next iteration
    }
    int *nullValue = NULL;
    copyout((void *) & nullValue, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy null into last position of argv

    md_usermode(nargs /*argc*/, (userptr_t) (stackptr + 4) /*userspace addr of argv*/, stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}
#else
#if OPT_A2

int runprogram(char *progname, char ** args, unsigned long nargs) {
    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result;

    /* Open the file. */
    result = vfs_open(progname, O_RDONLY, &v);
    if (result) {
        return result;
    }

    /* We should be a new thread. */
    assert(curthread->t_vmspace == NULL);

    /* Create a new address space. */
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) {
        vfs_close(v);
        return ENOMEM;
    }

    /* Activate it. */
    as_activate(curthread->t_vmspace);

    assert(curthread->t_vmspace != NULL);

    /* Load the executable. */
    result = load_elf(v, &entrypoint);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
        vfs_close(v);
        return result;
    }

    /* Done with the file now. */
    vfs_close(v);

    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
        return result;
    }

    /* calculate the size of the stack frame for the main() function */
    int stackFrameSize = 8; //minimum size for argc=0, argv[0]=NULL
    unsigned long i;
    for (i = 0; i < nargs; i++) {
        stackFrameSize += strlen(args[i]) + 1 + 4; //add the length of each argument, plus the space for the pointer
    }

    /* Decide the stackpointer address */
    stackptr -= stackFrameSize;
    //decrement the stackptr until it is divisible by 8
    for (; (stackptr % 8) > 0; stackptr--) {
    }

    /* copy the arguments to the proper location on the stack */
    int argumentStringLocation = (int) stackptr + 4 + ((nargs + 1) * 4); //begining of where strings will be stored
    copyout((void *) & nargs, (userptr_t) stackptr, (size_t) 4); //copy argc

    for (i = 0; i < nargs; i++) {
        copyout((void *) & argumentStringLocation, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy address of string into argv[i+1]
        copyoutstr(args[i], (userptr_t) argumentStringLocation, (size_t) strlen(args[i]), NULL); //copy the argument string
        argumentStringLocation += strlen(args[i]) + 1; //update the location for the next iteration
    }
    int *nullValue = NULL;
    copyout((void *) & nullValue, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy null into last position of argv

    md_usermode(nargs /*argc*/, (userptr_t) (stackptr + 4) /*userspace addr of argv*/, stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}
#else

int
runprogram(char *progname, char **argv, unsigned long argc) {
    (void) argv;
    (void) argc;

    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result;

    /* Open the file. */
    result = vfs_open(progname, O_RDONLY, &v);
    if (result) {
        return result;
    }

    /* We should be a new thread. */
    assert(curthread->t_vmspace == NULL);

    /* Create a new address space. */
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) {
        vfs_close(v);
        return ENOMEM;
    }

    /* Activate it. */
    as_activate(curthread->t_vmspace);

    /* Load the executable. */
    result = load_elf(v, &entrypoint);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
        vfs_close(v);
        return result;
    }

    /* Done with the file now. */
    vfs_close(v);

    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
        return result;
    }

    /* Warp to user mode. */
    md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/,
            stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}
#endif /* OPT_A2 */

#endif /* OPT_A3 */
