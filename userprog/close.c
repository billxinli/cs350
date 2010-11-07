/*
Name
close - close file

Library
Standard C Library (libc, -lc)

Synopsis
#include <unistd.h>

int
close(int fd);

Description
The file handle fd is closed. The same file handle may then be returned again from
open, dup2, pipe, or similar calls.

Other file handles are not affected in any way, even if they are attached to the
same file.

Return Values
On success, close returns 0. On error, -1 is returned, and errno is set according
to the error encountered.

Errors
The following error codes should be returned under the conditions given. Other
error codes may be returned for other errors not mentioned here.


    EBADF 	fd is not a valid file handle.
    EIO 	A hard I/O error occurred.
 */

#include "opt-A2.h"
#if OPT_A2
#include <types.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/unistd.h>
#include <lib.h>
#include <filetable.h>
#include <../arch/mips/include/spl.h>
#include <curthread.h>
#include <thread.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>

int sys_close(int *retval, int fdn) {

    if (ft_get(curthread->ft, fdn) == NULL) {
        return EBADF;
    }

    ft_remove(curthread->ft, fdn);
    *retval = 0;
    return 0;
}

#endif /* OPT_A2 */
