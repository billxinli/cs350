/*
Name
write - write data to file

Library
Standard C Library (libc, -lc)

Synopsis
#include <unistd.h>

int
write(int fd, const void *buf, size_t nbytes>);

Description
write writes up to buflen bytes to the file specified by fd, at the location in the file specified by the current seek position of the file, taking the data from the space pointed to by buf. The file must be open for writing.

The current seek position of the file is advanced by the number of bytes written.

Each write (or read) operation is atomic relative to other I/O to the same file.

Return Values
The count of bytes written is returned. This count should be positive. A return value of 0 means that nothing could be written, but that no error occurred; this only occurs at end-of-file on fixed-size objects. On error, write returns -1 and sets errno to a suitable error code for the error condition encountered.

Note that in some cases, particularly on devices, fewer than buflen (but greater than zero) bytes may be written. This depends on circumstances and does not necessarily signify end-of-file. In most cases, one should loop to make sure that all output has actually been written.

Errors
EBADF 	fd is not a valid file descriptor, or was not opened for writing.
EFAULT 	Part or all of the address space pointed to by buf is invalid.
ENOSPC 	There is no free space remaining on the filesystem containing the file.
EIO 	A hardware I/O error occurred writing the data.
*/

#include "opt-A2.h"

#if OPT_A2
#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>

int sys_write(int fd, const void *buf, size_t nbytes){

(void) fd;
(void) buf;
(void) nbytes;
return 1;

}


#endif /* OPT_A2 */


