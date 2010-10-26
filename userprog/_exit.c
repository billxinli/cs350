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

#include <unistd.h>

void sys__exit(int exitcode){
	kprintf("[SYSCALL] exiting with %d \n", exitcode);
	thread_exit();
}