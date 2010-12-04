/*
Name
read - read data from file

Library
Standard C Library (libc, -lc)

Synopsis
#include <unistd.h>

int
read(int fd, void *buf, size_t buflen);

Description
read reads up to buflen bytes from the file specified by fd, at the location in 
 * the file specified by the current seek position of the file, and stores them
 * in the space pointed to by buf. The file must be open for reading.

The current seek position of the file is advanced by the number of bytes read.

Each read (or write) operation is atomic relative to other I/O to the same file.

Return Values
The count of bytes read is returned. This count should be positive. A return
value of 0 should be construed as signifying end-of-file. On error, read
returns -1 and sets errno to a suitable error code for the error condition
encountered.

Note that in some cases, particularly on devices, fewer than buflen (but greater
than zero) bytes may be returned. This depends on circumstances and does not
necessarily signify end-of-file.

Errors
The following error codes should be returned under the conditions given. Other
error codes may be returned for other errors not mentioned here.
     	 
    EBADF 	fd is not a valid file descriptor, or was not opened for reading.
    EFAULT 	Part or all of the address space pointed to by buf is invalid.
    EIO 	A hardware I/O error occurred reading the data.
 */

#include "opt-A2.h"
#if OPT_A2
#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <filetable.h>
#include <../arch/mips/include/spl.h>
#include <addrspace.h>
#include <curthread.h>
#include <thread.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>
#include <vm.h>

int sys_read(int *retval, int fdn, void *buf, size_t nbytes) {
    //Check for Bad memory reference.
    if (!buf || (u_int32_t) buf >= MIPS_KSEG0 || !as_valid_write_addr(curthread->t_vmspace,buf)) {
        return EFAULT;
    }
    //Get the file descriptor from the opened list of file descriptors that the current thread has, based on the fdn given.
    struct filedescriptor* fd = ft_get(curthread->ft, fdn);
    if (fd == NULL) {
        return EBADF;
    }
    //Make sure that the file is opened for writing.
    switch (O_ACCMODE & fd->mode) {
        case O_RDONLY:
        case O_RDWR:
            break;
        default:
            return EBADF;
    }
    //Make the uio
    struct uio u;
    mk_kuio(&u, (void *) buf, nbytes, fd->offset, UIO_READ);
    //Disable interrupt
    //int spl;
    //spl = splhigh();
    //Read
    int sizeread = VOP_READ(fd->fdvnode, &u);
    //splx(spl);
    if (sizeread) {
        return sizeread;
    }
    //Find the number of bytes read
    sizeread = nbytes - u.uio_resid;
    *retval = sizeread;
    //Update the offset
    fd->offset += sizeread;
    return 0;
}

#endif /* OPT_A2 */
