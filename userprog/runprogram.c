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

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */

int runprogram(char *progname, char ** args, unsigned long nargs) {
    //   (void) args;
    //  (void) nargs;
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

    /* fetch the arguments and environment from the caller */

    int i;
    size_t copystrlen;

    char **p_args;
    for (i = 0; i < nargs; i++) {
        kprintf("=====%s=====\n", args[i]);
        copyinstr((userptr_t) args[i], p_args[i], sizeof (args[i]), &copystrlen);
        kprintf("=====%s=====\n", p_args[i]);
    }

    p_args[nargs] = NULL;
    copyin((userptr_t) args, &stackptr, sizeof (args));
    kprintf("Stack Pointer: %p \nEntry point: %p\n", &stackptr, &entrypoint);

    for (i = 0; i < nargs; i++) {
        kprintf("=====%p=====\n", &p_args[i]);
        kprintf("=====%s=====\n", p_args[i]);
    }

    /* Warp to user mode. */
    md_usermode(nargs, (userptr_t) p_args /*userspace addr of argv*/, stackptr, entrypoint);

    md_usermode(nargs /*argc*/, NULL /*userspace addr of argv*/, stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}

int runprogram2(char *progname, char ** args, unsigned long nargs) {
    //   (void) args;
    //  (void) nargs;
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

    md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/, stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}

