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
    //first we need to calculate the size of args
    int nargs = 0;
    char* argumentPointer = args;
    while(*argumentPointer != NULL){
        nargs++;
        argumentPointer += sizeof(char*);
    }
    
    
    struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should not be a new thread. Should already have an address space */
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
	int i;
	for(i = 0;i < nargs;i++){
		stackFrameSize += strlen(args[i]) + 1 + 4; //add the length of each argument, plus the space for the pointer
	}

	/* Decide the stackpointer address */
	stackptr -= stackFrameSize;
	for(;(stackptr % 8) > 0;stackptr--){} //decrement the stackptr until it is divisible by 8
	
	/* copy the arguments to the proper location on the stack */
	int argumentStringLocation = (int)stackptr + 4 + ((nargs+1) * 4); //begining of where strings will be stored
	copyout((void *)&nargs, (userptr_t) stackptr, (size_t) 4);	//copy argc
	
	for(i=0;i<nargs;i++){
		copyout((void *)&argumentStringLocation, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy address of string into argv[i+1]
		copyoutstr(args[i], (userptr_t) argumentStringLocation, (size_t) strlen(args[i]), NULL); //copy the argument string
		argumentStringLocation += strlen(args[i]) + 1;  //update the location for the next iteration
	}
	int nullValue = NULL;
	copyout((void *)&nullValue, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy null into last position of argv
	
	md_usermode(nargs /*argc*/, (stackptr + 4) /*userspace addr of argv*/, stackptr, entrypoint);

	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}
#endif /* OPT_A2 */
