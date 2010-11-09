/*
Name
open - open a file

Library
Standard C Library (libc, -lc)

Synopsis
#include <unistd.h>

int
open(const char *filename, int flags);
int
open(const char *filename, int flags, int mode);

Description
open opens the file, device, or other kernel object named by the pathname filename.
The flags argument specifies how to open the file. The optional mode argument
is only meaningful in Unix (or if you choose to implement Unix-style security later on)
and can be ignored.

The flags argument should consist of one of

    O_RDONLY 	Open for reading only.
    O_WRONLY 	Open for writing only.
    O_RDWR 	Open for reading and writing.

It may also have any of the following flags OR'd in:

    O_CREAT 	Create the file if it doesn't exist.
    O_EXCL 	Fail if the file already exists.
    O_TRUNC 	Truncate the file to length 0 upon open.
    O_APPEND 	Open the file in append mode.

O_EXCL is only meaningful if O_CREAT is also used.

O_APPEND causes all writes to the file to occur at the end of file, no matter
what gets written to the file by whoever else. (This functionality may be
optional; consult your course's assignments.)

open returns a file handle suitable for passing to read, write, close, etc.
This file handle must be greater than or equal to zero. Note that file handles
0 (STDIN_FILENO), 1 (STDOUT_FILENO), and 2 (STDERR_FILENO) are used in special
ways and are typically assumed by user-level code to always be open.

Return Values
On success, open returns a nonnegative file handle. On error, -1 is returned,
and errno is set according to the error encountered.

Errors
The following error codes should be returned under the conditions given. Other error codes may be returned for other errors not mentioned here.


    ENODEV 	The device prefix of filename did not exist.
    ENOTDIR 	A non-final component of filename was not a directory.
    ENOENT 	A non-final component of filename did not exist.
    ENOENT 	The named file does not exist, and O_CREAT was not specified.
    EEXIST 	The named file exists, and O_EXCL was specified.
    EISDIR 	The named object is a directory, and it was to be opened for writing.
    EMFILE 	The process's file table was full, or a process-specific limit on open files was reached.
    ENFILE 	The system file table is full, if such a thing exists, or a system-wide limit on open files was reached.
    ENXIO 	The named object is a block device with no mounted filesystem.
    ENOSPC 	The file was to be created, and the filesystem involved is full.
    EINVAL 	flags contained invalid values.
    EIO 	A hard I/O error occurred.
    EFAULT 	filename was an invalid pointer.
 */

#include "opt-A2.h"
#if OPT_A2
#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/stat.h>
#include <lib.h>
#include <addrspace.h>
#include <filetable.h>
#include <../arch/mips/include/spl.h>
#include <curthread.h>
#include <thread.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>

int sys_open(int *retval, char *filename, int flags, int mode) {

    if(!as_valid_read_addr(curthread->t_vmspace, (vaddr_t *)filename)){
        return EFAULT;
    }

    if (flags >= 63 || strlen(filename) < 1) {
        return EINVAL;
    }

    (void) mode;
    struct filedescriptor* fd = kmalloc(sizeof (struct filedescriptor));
    fd->fdvnode = kmalloc(sizeof (struct vnode));
    char *kfilename = kstrdup(filename);
    int copyflag = flags;
    flags = flags&O_ACCMODE;
    int offset = 0;
    int result = vfs_open(kfilename, copyflag, &fd->fdvnode);
    if (result) {
        return result;
    }

    if (copyflag & O_APPEND) {
        struct stat *statbuf = kmalloc(sizeof ( struct stat));
        VOP_STAT(fd->fdvnode, statbuf);
        offset = statbuf->st_size;
        kfree(statbuf);
    }

    if (copyflag & O_TRUNC) {
        VOP_TRUNCATE(fd->fdvnode, 0);
    }

    kfree(kfilename);
    fd->offset = offset;
    fd->mode = flags;
    fd->numOwners = 0; //will increment to 1 upon ft_add
    result = ft_add(curthread->ft, fd);
    *retval = result;
    return 0;
}
#endif /* OPT_A2 */
