#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "opt-A2.h"
#if OPT_A2
#include <types.h>
#include <machine/trapframe.h>
#endif

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
#if OPT_A2
void sys__exit(int exitcode);
int sys_write(int *retval, int filehandle, const void *buf, size_t size);
int sys_read(int *retval, int filehandle, const void *buf, size_t size);
int sys_open(int *retval, char *filename, int flags, int modes);
int sys_close(int *retval, int fdn);
pid_t sys_getpid();
int sys_waitpid(pid_t PID, int *status, int options);
pid_t sys_fork(struct trapframe *tf);
int sys_execv(char *progname, char ** args);
#endif




#endif /* _SYSCALL_H_ */
