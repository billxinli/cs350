/*
Name
        execv - execute a program


Library
        Standard C Library (libc, -lc)


Synopsis
        #include <unistd.h>
        int
        execv(const char *program, char **args);


Description
        execv replaces the currently executing program with a newly loaded program image. This occurs within one process; the process id is unchanged.
        The pathname of the program to run is passed as program. The args argument is an array of 0-terminated strings. The array itself should be terminated by a NULL pointer.

        The argument strings should be copied into the new process as the new process's argv[] array. In the new process, argv[argc] must be NULL.

        By convention, argv[0] in new processes contains the name that was used to invoke the program. This is not necessarily the same as program, and furthermore is only a convention and should not be enforced by the kernel.

        The process file table and current working directory are not modified by execve.


Return Values
        On success, execv does not return; instead, the new program begins executing. On failure, execv returns -1, and sets errno to a suitable error code for the error condition encountered.


Errors
        The following error codes should be returned under the conditions given. Other error codes may be returned for other errors not mentioned here.
	 	 
        ENODEV	The device prefix of program did not exist.
        ENOTDIR	A non-final component of program was not a directory.
        ENOENT	program did not exist.
        EISDIR	program is a directory.
        ENOEXEC	program is not in a recognizable executable file format, was for the wrong platform, or contained invalid fields.
        ENOMEM	Insufficient virtual memory is available.
        E2BIG	The total size of the argument strings is too large.
        EIO	A hard I/O error occurred.
        EFAULT	One of the args is an invalid pointer.
 */


#include "opt-A2.h"
#include "opt-A3.h"
#if OPT_A3
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

int sys_execv(char *progname, char ** args) {
    DEBUG(DB_EXEC, "EXECV[%d] Entered EXECV - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, progname, (int) curthread->t_vmspace);

    /* We should be a an existing thread. */
    assert(curthread->t_vmspace != NULL);

    //check the arguments are valid pointers
    if (!(as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) progname) && as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) args))) {
        return EFAULT;
    }
    if (strlen(progname) < 1) {
        return EINVAL;
    }

    /* We need to copy the programname and args into kernel space */
    //first we need to calculate the size of the args
    int nargs = 0;
    int i = 0;
    while (args[i] != NULL) {
        nargs++;
        i++;
    }

    //check that we don't have too many arguments
    if (nargs > 16) { //same as MAXMENUARGS from the menu
        return E2BIG;
    }

    //allocate space for copied progname
    char* kern_progname = kmalloc(sizeof (char) * (strlen(progname) + 1));
    //copy the programname
    copyinstr((const_userptr_t) progname, kern_progname, (strlen(progname) + 1), NULL);

    //allocate space for copied args array
    char** kern_argumentPointer = kmalloc(sizeof (char*) * nargs);
    //allocate the space for the argument strings and copy them into kernel memory
    for (i = 0; i < nargs; i++) {
        if (!as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) args[i])) {
            return EFAULT;
        }
        if (strlen(args[i]) < 1) {
            return EINVAL;
        }
        int arg_size = strlen(args[i]);
        kern_argumentPointer[i] = kmalloc(sizeof (char) * (arg_size + 1));
        copyinstr((const_userptr_t) args[i], kern_argumentPointer[i], (arg_size + 1), NULL);
    }

    vaddr_t entrypoint, stackptr;
    int result;


    DEBUG(DB_EXEC, "EXECV[%d] Before AS_DESTROY - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);
    /* Destory ourcurrent address space */
    as_destroy(curthread->t_vmspace);
    curthread->t_vmspace = NULL;
    /* Now our address space should be NULL */
    assert(curthread->t_vmspace == NULL);
    DEBUG(DB_EXEC, "EXECV[%d] After AS_DESTROY - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);

    DEBUG(DB_EXEC, "EXECV[%d] Before AS_CREATE - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);
    /* Create a new address space. */
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) {

        return ENOMEM;
    }

    DEBUG(DB_EXEC, "EXECV[%d] After AS_CREATE - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);
    /* Activate it. */
    as_activate(curthread->t_vmspace);

    assert(curthread->t_vmspace != NULL);

    DEBUG(DB_EXEC, "EXECV[%d] Before LOAD_ELF - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);
    /* Load the executable. */
    result = load_elf(kern_progname, &entrypoint);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */

        return result;
    }
    DEBUG(DB_EXEC, "EXECV[%d] After LOAD_ELF - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);



    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
        return result;
    }

    /* calculate the size of the stack frame for the main() function */
    int stackFrameSize = 8; //minimum size for argc=0, argv[0]=NULL
    for (i = 0; i < nargs; i++) {
        stackFrameSize += strlen(kern_argumentPointer[i]) + 1 + 4; //add the length of each argument, plus the space for the pointer
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
        copyoutstr(kern_argumentPointer[i], (userptr_t) argumentStringLocation, (size_t) strlen(kern_argumentPointer[i]), NULL); //copy the argument string
        argumentStringLocation += strlen(kern_argumentPointer[i]) + 1; //update the location for the next iteration
    }
    int *nullValue = NULL;
    copyout((void *) & nullValue, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy null into last position of argv

    /* free up the memory we copied our arguments to in the kernel */
    kfree(kern_progname);

    //free up each string in the arguments
    for (i = 0; i < nargs; i++) {
        kfree(kern_argumentPointer[i]);
    }
    kfree(kern_argumentPointer);

    DEBUG(DB_EXEC, "EXECV[%d] Leaving EXECV (md_usermode) - nargs:[%d]\n", (int) curthread->pid, nargs);
    md_usermode(nargs /*argc*/, (userptr_t) (stackptr + 4) /*userspace addr of argv*/, stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}
#else
#if OPT_A2
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

int sys_execv(char *progname, char ** args) {
    DEBUG(DB_EXEC, "EXECV[%d] Entered EXECV - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, progname, (int) curthread->t_vmspace);

    /* We should be a an existing thread. */
    assert(curthread->t_vmspace != NULL);

    //check the arguments are valid pointers
    if (!(as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) progname) && as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) args))) {
        return EFAULT;
    }
    if (strlen(progname) < 1) {
        return EINVAL;
    }

    /* We need to copy the programname and args into kernel space */
    //first we need to calculate the size of the args
    int nargs = 0;
    int i = 0;
    while (args[i] != NULL) {
        nargs++;
        i++;
    }

    //check that we don't have too many arguments
    if (nargs > 16) { //same as MAXMENUARGS from the menu
        return E2BIG;
    }

    //allocate space for copied progname
    char* kern_progname = kmalloc(sizeof (char) * (strlen(progname) + 1));
    //copy the programname
    copyinstr((const_userptr_t) progname, kern_progname, (strlen(progname) + 1), NULL);

    //allocate space for copied args array
    char** kern_argumentPointer = kmalloc(sizeof (char*) * nargs);
    //allocate the space for the argument strings and copy them into kernel memory
    for (i = 0; i < nargs; i++) {
        if (!as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) args[i])) {
            return EFAULT;
        }
        if (strlen(args[i]) < 1) {
            return EINVAL;
        }
        int arg_size = strlen(args[i]);
        kern_argumentPointer[i] = kmalloc(sizeof (char) * (arg_size + 1));
        copyinstr((const_userptr_t) args[i], kern_argumentPointer[i], (arg_size + 1), NULL);
    }

    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result;

    /* Open the file. */
    result = vfs_open(kern_progname, O_RDONLY, &v);
    if (result) {
        return result;
    }



    DEBUG(DB_EXEC, "EXECV[%d] Before AS_DESTROY - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);
    /* Destory ourcurrent address space */
    as_destroy(curthread->t_vmspace);
    curthread->t_vmspace = NULL;
    /* Now our address space should be NULL */
    assert(curthread->t_vmspace == NULL);
    DEBUG(DB_EXEC, "EXECV[%d] After AS_DESTROY - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);

    DEBUG(DB_EXEC, "EXECV[%d] Before AS_CREATE - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);
    /* Create a new address space. */
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) {
        vfs_close(v);
        return ENOMEM;
    }

    DEBUG(DB_EXEC, "EXECV[%d] After AS_CREATE - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);
    /* Activate it. */
    as_activate(curthread->t_vmspace);

    assert(curthread->t_vmspace != NULL);

    DEBUG(DB_EXEC, "EXECV[%d] Before LOAD_ELF - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);
    /* Load the executable. */
    result = load_elf(v, &entrypoint);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
        vfs_close(v);
        return result;
    }
    DEBUG(DB_EXEC, "EXECV[%d] After LOAD_ELF - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);


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
    for (i = 0; i < nargs; i++) {
        stackFrameSize += strlen(kern_argumentPointer[i]) + 1 + 4; //add the length of each argument, plus the space for the pointer
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
        copyoutstr(kern_argumentPointer[i], (userptr_t) argumentStringLocation, (size_t) strlen(kern_argumentPointer[i]), NULL); //copy the argument string
        argumentStringLocation += strlen(kern_argumentPointer[i]) + 1; //update the location for the next iteration
    }
    int *nullValue = NULL;
    copyout((void *) & nullValue, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy null into last position of argv

    /* free up the memory we copied our arguments to in the kernel */
    kfree(kern_progname);

    //free up each string in the arguments
    for (i = 0; i < nargs; i++) {
        kfree(kern_argumentPointer[i]);
    }
    kfree(kern_argumentPointer);

    DEBUG(DB_EXEC, "EXECV[%d] Leaving EXECV (md_usermode) - nargs:[%d]\n", (int) curthread->pid, nargs);
    md_usermode(nargs /*argc*/, (userptr_t) (stackptr + 4) /*userspace addr of argv*/, stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}
#endif /* OPT_A2 */
#endif /* OPT_A3 */
