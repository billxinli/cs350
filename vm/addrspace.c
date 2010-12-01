#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <vmstats.h>
#include <vm_tlb.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include "opt-A3.h"

#if OPT_A3
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <vm_tlb.h>
#include <pt.h>
#include <coremap.h>


/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

void
vm_bootstrap(void) {
    /* Do nothing. */
}

static paddr_t getppages(unsigned long npages) {
    assert(0); //this should not be called anymore
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(int npages) {
    return cm_request_kframes(npages);
}

void
free_kpages(vaddr_t addr) {
    assert(addr % PAGE_SIZE == 0);
    cm_release_kframes(addr / PAGE_SIZE);
}

int
vm_fault(int faulttype, vaddr_t faultaddress) {
    vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
    paddr_t paddr;
    struct addrspace *as;
    int spl;

    spl = splhigh();

    faultaddress &= PAGE_FRAME;

    DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

    struct page_detail *pd = pt_getpdetails((faultaddress / PAGE_SIZE), curthread);

    if (pd == NULL) {
        thread_exit();
    }

    switch (faulttype) {
        case VM_FAULT_READONLY:
            if (pt_checkreadonly(pd)) {
                thread_exit();
                /* We always create pages read-write, so we can't get this */
                panic("dumbvm: got VM_FAULT_READONLY\n");
            }
            break;
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:


            if (pd->valid) {
                tlb_add_entry(faultaddress, pd->pfn*PAGE_SIZE, pd->dirty);
                pd->use = 1;
                if (faulttype == VM_FAULT_WRITE) {
                    pd->modified = 1;
                }
                splx(spl);
                return 0;
            }

            //page fault


            break;
        default:
            splx(spl);
            return EINVAL;
    }

    as = curthread->t_vmspace;
    if (as == NULL) {
        /*
         * No address space set up. This is probably a kernel
         * fault early in boot. Return EFAULT so as to panic
         * instead of getting into an infinite faulting loop.
         */
        return EFAULT;
    }

    /* Assert that the address space has been set up properly. */
    tlb_add_entry(faultaddress, paddr, 1);
    splx(spl);
    return 0;
}

struct addrspace *
as_create(void) {
    struct addrspace *as = kmalloc(sizeof (struct addrspace));
    if (as == NULL) {
        return NULL;
    }
    //as->pt = NULL;
    as->file = NULL;
    return as;
}

void
as_destroy(struct addrspace *as) {
    //pt_destroy(as->pt);
    kfree(as);
}

void
as_activate(struct addrspace *as) {
    tlb_context_switch();
    (void) as;
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
        int readable, int writeable, int executable) {
    size_t npages;

    /* Align the region. First, the base... */
    sz += vaddr & ~(vaddr_t) PAGE_FRAME;
    vaddr &= PAGE_FRAME;

    /* ...and now the length. */
    sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

    npages = sz / PAGE_SIZE;

    /* We don't use these - all pages are read */
    (void) readable;
    (void) executable;

    if (as->segments[0].vbase == 0) {
        as->segments[0].vbase = vaddr;
        as->segments[0].size = npages;
        as->segments[0].writeable = writeable;
        return 0;
    }

    if (as->segments[1].vbase == 0) {
        as->segments[1].vbase = vaddr;
        as->segments[1].size = npages;
        as->segments[1].writeable = writeable;
        return 0;
    }





    /*
     * Support for more than two regions is not available.
     */
    kprintf("addrspace: Warning: too many regions\n");
    return EUNIMP;
}

int
as_prepare_load(struct addrspace *as) {

    (void) as;


    return 0;
}

int
as_complete_load(struct addrspace *as) {
    (void) as;
    return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
    assert(as->segments[2].vbase == 0);


    as->segments[2].vbase = USERSTACK - PAGE_SIZE*DUMBVM_STACKPAGES;
    as->segments[2].size = DUMBVM_STACKPAGES;

    as->segments[2].writeable = 1;


    //as->segments[2].p_offset; /* Location of data within file */
    //as->segments[2].p_filesz; /* Size of data within file */
    //as->segments[2].p_memsz; /* Size of data to be loaded into memory*/
    //as->segments[2].p_flags; /* Flags */



    *stackptr = USERSTACK;
    return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret) {
    struct addrspace *new;

    new = as_create();
    if (new == NULL) {
        return ENOMEM;
    }

    new->as_vbase1 = old->as_vbase1;
    new->as_npages1 = old->as_npages1;
    new->as_vbase2 = old->as_vbase2;
    new->as_npages2 = old->as_npages2;

    if (as_prepare_load(new)) {
        as_destroy(new);
        return ENOMEM;
    }

    assert(new->as_pbase1 != 0);
    assert(new->as_pbase2 != 0);
    assert(new->as_stackpbase != 0);

    memmove((void *) PADDR_TO_KVADDR(new->as_pbase1),
            (const void *) PADDR_TO_KVADDR(old->as_pbase1),
            old->as_npages1 * PAGE_SIZE);

    memmove((void *) PADDR_TO_KVADDR(new->as_pbase2),
            (const void *) PADDR_TO_KVADDR(old->as_pbase2),
            old->as_npages2 * PAGE_SIZE);

    memmove((void *) PADDR_TO_KVADDR(new->as_stackpbase),
            (const void *) PADDR_TO_KVADDR(old->as_stackpbase),
            DUMBVM_STACKPAGES * PAGE_SIZE);

    *ret = new;
    return 0;
}

int as_valid_read_addr(struct addrspace *as, vaddr_t *check_addr) {
    if (check_addr < (vaddr_t*) USERTOP) {
        if (check_addr >= (vaddr_t*) as->as_vbase1 && check_addr < (vaddr_t*) (as->as_vbase1 + as->as_npages1 * PAGE_SIZE)) {
            return 1;
        }
        if (check_addr >= (vaddr_t*) as->as_vbase2 && check_addr < (vaddr_t*) (as->as_vbase2 + as->as_npages2 * PAGE_SIZE)) {
            return 1;
        }
        if (check_addr >= (vaddr_t*) (USERTOP - DUMBVM_STACKPAGES * PAGE_SIZE)) {
            return 1;
        }
    }
    return 0;
}

int as_valid_write_addr(struct addrspace *as, vaddr_t *check_addr) {
    if (check_addr < (vaddr_t*) USERTOP) {
        if (check_addr >= (vaddr_t*) as->as_vbase2 && check_addr < (vaddr_t*) (as->as_vbase2 + as->as_npages2 * PAGE_SIZE)) {
            return 1;
        }
        if (check_addr >= (vaddr_t*) (USERTOP - DUMBVM_STACKPAGES * PAGE_SIZE)) {
            return 1;
        }
    }
    return 0;
}


#else

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void) {
    struct addrspace *as = kmalloc(sizeof (struct addrspace));
    if (as == NULL) {
        return NULL;
    }

    /*
     * Initialize as needed.
     */

    return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret) {
    struct addrspace *newas;

    newas = as_create();
    if (newas == NULL) {
        return ENOMEM;
    }

    /*
     * Write this.
     */

    (void) old;

    *ret = newas;
    return 0;
}

void
as_destroy(struct addrspace *as) {
    /*
     * Clean up as needed.
     */

    kfree(as);
}

void
as_activate(struct addrspace *as) {
    /*
     * Write this.
     */

    (void) as; // suppress warning until code gets written
}

void as_destroy(struct addrspace *as) {
    kfree(as);
}

void as_activate(struct addrspace *as) {
    (void) as;
    tlb_context_switch();

}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
        int readable, int writeable, int executable) {
    /*
     * Write this.
     */

    (void) as;
    (void) vaddr;
    (void) sz;
    (void) readable;
    (void) writeable;
    (void) executable;
    return EUNIMP;
}

int
as_prepare_load(struct addrspace *as) {
    /*
     * Write this.
     */

    (void) as;
    return 0;
}

int
as_complete_load(struct addrspace *as) {
    /*
     * Write this.
     */

    (void) as;
    return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
    /*
     * Write this.
     */

    (void) as;

    /* Initial user-level stack pointer */
    *stackptr = USERSTACK;

    return 0;
}

#if OPT_A2

int as_valid_read_addr(struct addrspace *as, vaddr_t *check_addr) {
    (void) as;
    (void) check_addr;
    /* write this */
    return 0;
}

int as_valid_write_addr(struct addrspace *as, vaddr_t *check_addr) {
    (void) as;
    (void) check_addr;
    /* write this */
    return 0;
}
#endif /* OPT_A2 */

#endif /* OPT_A3 */
