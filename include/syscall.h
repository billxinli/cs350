#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "opt-A2.h"
#if OPT_A2
#include <types.h>
#endif

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
#if OPT_A2
void sys__exit(int exitcode);
int sys_write(int *retval, int filehandle, const void *buf, size_t size);
int sys_read(int *retval, int filehandle, const void *buf, size_t size);
pid_t sys_getpid();
#endif




#endif /* _SYSCALL_H_ */
