#include "opt-A3.h"

#include <types.h>
#include <kern/errmsg.h>
#include <lib.h>

#if OPT_A3
#include <vm.h>
#endif

/*
 * Like strdup, but calls kmalloc.
 */
char *
kstrdup(const char *s)
{
    #if OPT_A3
    if (is_vm_setup()) {
        char *z = kmalloc(strlen(s)+1);
    } else {
        char *z = ralloc(strlen(s)+1);
    }
    #else
	char *z = kmalloc(strlen(s)+1);
	#endif
	if (z==NULL) {
		return NULL;
	}
	strcpy(z, s);
	return z;
}

/*
 * Standard C function to return a string for a given errno.
 * Kernel version; panics if it hits an unknown error.
 */
const char *
strerror(int errcode)
{
	if (errcode>=0 && errcode < sys_nerr) {
		return sys_errlist[errcode];
	}
	panic("Invalid error code %d\n", errcode);
	return NULL;
}
