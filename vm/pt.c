#include "opt-A3.h"

#if OPT_A3

#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <machine/spl.h>
#include <lib.h>
#include <array.h>
#include <queue.h>
#include <vfs.h>
#include <vnode.h>
#include <thread.h>
#include <vm.h>
#include <swapfile.h>
#include <pt.h>

struct page_table* pt_init() {
    struct page_table * pt = kmalloc(sizeof (struct page_table));
    if (pt == NULL) {
        //Panic?
        return NULL;
    }
    return pt;
}

#endif /* OPT_A3 */
