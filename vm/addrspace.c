#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>

#include "opt-A2.h"
#include "opt-A3.h"

#if OPT_A3
#include <thread.h>
#include <curthread.h>
#include <segments.h>
#include <vm_tlb.h>
#include <vmstats.h>
#include <machine/spl.h>
#include <machine/tlb.h>

#include <elf.h>
/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

void vm_bootstrap(void) {
    vmstats_init();
}

void vm_shutdown(void) {
    int spl = splhigh();
    _vmstats_print();
    splx(spl);
}

static paddr_t getppages(unsigned long npages) {
    int spl;
    paddr_t addr;

    spl = splhigh();

    addr = ram_stealmem(npages);

    splx(spl);
    return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t alloc_kpages(int npages) {
    paddr_t pa;
    pa = getppages(npages);
    if (pa == 0) {
        return 0;
    }
    return PADDR_TO_KVADDR(pa);
}

void free_kpages(vaddr_t addr) {
    /* nothing */

    (void) addr;
}

int vm_fault(int faulttype, vaddr_t faultaddress) {
    paddr_t paddr;
    struct addrspace *as;
    int spl;

    spl = splhigh();

    faultaddress &= PAGE_FRAME;

    as = curthread->t_vmspace;
    if (as == NULL) {
        /*
         * No address space set up. This is probably a kernel
         * fault early in boot. Return EFAULT so as to panic
         * instead of getting into an infinite faulting loop.
         */
        return EFAULT;
    }

    switch (faulttype) {
        case VM_FAULT_READONLY:
            /* We always create pages read-write, so we can't get this */

            DEBUG(DB_ELF, "ELF: VM_FAULT_READONLY\n");
            thread_exit();
            return EFAULT;
        case VM_FAULT_WRITE:
            if (!as_valid_write_addr(as, (void *) faultaddress)) {

                DEBUG(DB_ELF, "ELF: VM_FAULT_WRITE\n");
                thread_exit();
                return EFAULT;
            }
            break;
        case VM_FAULT_READ:
            if (!as_valid_read_addr(as, (void *) faultaddress)) {

                DEBUG(DB_ELF, "ELF: VM_FAULT_READ on %x\n", faultaddress);
                thread_exit();
                return EFAULT;
            }
            break;
        default:
            splx(spl);
            return EINVAL;
    }

    struct segment * s = as_get_segment(as, faultaddress);
    load_segment_page(as->file, faultaddress, s);

    _vmstats_inc(VMSTAT_TLB_FAULT);

    int writeable;

    //TODO: pbase
    paddr = (faultaddress - s->vbase + s->pbase) & PAGE_FRAME;
    writeable = s->writeable;

    DEBUG(DB_ELF, "TLB ADDING ENTRY: %x -> %x\n", faultaddress, paddr);
    tlb_add_entry(faultaddress, paddr, writeable, 1);

    splx(spl);
    return 0;
}

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */


struct addrspace * as_create(void) {
    struct addrspace *as = kmalloc(sizeof (struct addrspace));
    if (as == NULL) {
        return NULL;
    }

    int i;
    for (i = 0; i < AS_NUM_SEG - 1; i++) {
        as->segments[i].active = 0;
        as->segments[i].vbase = 0;

        //TODO: Remove
        as->segments[i].pbase = 0;
        as->segments[i].size = 0;
        as->segments[i].writeable = 0;
        as->segments[i].p_offset = 0;
        as->segments[i].p_filesz = 0;
        as->segments[i].p_memsz = 0;
        as->segments[i].p_flags = 0;

    }





    as->file = NULL;
    as->num_segments = 0;
    return as;
}

void
as_destroy(struct addrspace *as) {

    kfree(as);
}

void
as_activate(struct addrspace *as) {


    (void) as;

    //tlb_context_switch();
}

int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz, int flags, u_int32_t offset, u_int32_t filesz) {
    size_t npages;
    size_t memsz = sz;
    /* Align the region. First, the base... */
    sz += vaddr & ~(vaddr_t) PAGE_FRAME;
    vaddr &= PAGE_FRAME;

    /* ...and now the length. */
    sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

    npages = sz / PAGE_SIZE;

    //DEBUG(DB_ELF, "ELF: Define seg: %d \n", as->num_segments);

    if (as->num_segments < AS_NUM_SEG - 1) {
        assert(as->segments[as->num_segments].active == 0);
        as->segments[as->num_segments].active = 1;
        as->segments[as->num_segments].vbase = vaddr;
        as->segments[as->num_segments].size = npages;
        as->segments[as->num_segments].writeable = flags & PF_W;
        as->segments[as->num_segments].p_offset = offset;
        as->segments[as->num_segments].p_memsz = memsz;
        as->segments[as->num_segments].p_filesz = filesz;
        as->segments[as->num_segments].p_flags = flags & PF_X;


        //DEBUG(DB_ELF, "\t SEG: %d vbase: 0x%x  \n", as->num_segments, as->segments[as->num_segments].vbase);
        //DEBUG(DB_ELF, "\t SEG: %d size: %d \n", as->num_segments, npages);
        //DEBUG(DB_ELF, "\t SEG: %d writeable: %d \n", as->num_segments, flags & PF_W);
        //DEBUG(DB_ELF, "\t SEG: %d p_offset: %d \n", as->num_segments, offset);
        //DEBUG(DB_ELF, "\t SEG: %d p_memsz: %d \n", as->num_segments, memsz);
        //DEBUG(DB_ELF, "\t SEG: %d p_filesz: %d \n", as->num_segments, filesz);
        //DEBUG(DB_ELF, "\t SEG: %d p_flags: %d \n", as->num_segments, flags & PF_X);



        as->num_segments++;
        return 0;
    } else {
        /*
         * Support for more than AS_NUM_SEG regions is not available.
         */
        kprintf("dumbvm: Warning: too many regions\n");

        return EUNIMP;
    }
    //Keeps the compiler happy
    return 0;

}

int as_prepare_load(struct addrspace *as) {
    int i;
    for (i = 0; i < AS_NUM_SEG - 1; i++) {
        if (as->segments[i].active) {
            //TODO: remove
            assert(as->segments[i].pbase == 0);
            as->segments[i].pbase = getppages(as->segments[i].size);

            //DEBUG(DB_ELF, "ELF: pbase: %x\n", as->segments[i].pbase);
            if (as->segments[i].pbase == 0) {
                return ENOMEM;
            }
        }

    }
    return 0;
}

int as_complete_load(struct addrspace *as) {
    (void) as;

    return 0;
}

int as_copy(struct addrspace *old, struct addrspace **ret) {

    assert(0);
    struct addrspace *new;

    new = as_create();
    if (new == NULL) {
        return ENOMEM;
    }

    int i = 0;
    for (i = 0; i < AS_NUM_SEG; i++) {

        if (old->segments[i].active) {
            new->segments[i].active = 1;
            new->segments[i].vbase = old->segments[i].vbase;
            new->segments[i].size = old->segments[i].size;

            new->segments[i].writeable = old->segments[i].writeable;
            new->segments[i].p_offset = old->segments[i].p_offset;
            new->segments[i].p_filesz = old->segments[i].p_filesz;
            new->segments[i].p_memsz = old->segments[i].p_memsz;
            new->segments[i].p_flags = old->segments[i].p_flags;
        }
    }


    new->file = old->file;
    new->num_segments = old->num_segments;

    if (as_prepare_load(new)) {
        as_destroy(new);
        return ENOMEM;
    }

    for (i = 0; i < AS_NUM_SEG; i++) {
        if (new->segments[i].active) {
            //TODO: remove
            assert(new->segments[i].pbase != 0);
        }
    }

    for (i = 0; i < AS_NUM_SEG; i++) {
        //TODO:remove/fix
        memmove((void *) PADDR_TO_KVADDR(new->segments[i].pbase),
                (const void *) PADDR_TO_KVADDR(old->segments[i].pbase),
                old->segments[i].size * PAGE_SIZE);

    }
    *ret = new;

    return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
    //assert(as->segments[AS_NUM_SEG - 1].active);

    /* Initial user-level stack pointer */
    // *stackptr = as->segments[AS_NUM_SEG - 1].vbase + as->segments[AS_NUM_SEG - 1].size*PAGE_SIZE;


    as->segments[AS_NUM_SEG - 1].active = 1;
    as->segments[AS_NUM_SEG - 1].vbase = USERTOP - DUMBVM_STACKPAGES*PAGE_SIZE;

    //TODO: Remove
    as->segments[AS_NUM_SEG - 1].pbase = 0;
    as->segments[AS_NUM_SEG - 1].size = DUMBVM_STACKPAGES;
    as->segments[AS_NUM_SEG - 1].writeable = 1;
    as->segments[AS_NUM_SEG - 1].p_offset = 0;
    as->segments[AS_NUM_SEG - 1].p_filesz = 0;
    as->segments[AS_NUM_SEG - 1].p_memsz = 0;
    as->segments[AS_NUM_SEG - 1].p_flags = 0;

    //DEBUG(DB_ELF, "\t SEG: %d vbase: 0x%x  \n", 2, as->segments[2].vbase);
    //DEBUG(DB_ELF, "\t SEG: %d size: %d \n", 2, as->segments[2].size);
    //DEBUG(DB_ELF, "\t SEG: %d writeable: %d \n", as->num_segments, flags & PF_W);
    //DEBUG(DB_ELF, "\t SEG: %d p_offset: %d \n", 2, as->segments[2].p_offset);
    //DEBUG(DB_ELF, "\t SEG: %d p_memsz: %d \n", as->num_segments, memsz);
    //DEBUG(DB_ELF, "\t SEG: %d p_filesz: %d \n", 2, as->segments[2].p_filesz);
    //DEBUG(DB_ELF, "\t SEG: %d p_flags: %d \n", as->num_segments, flags & PF_X);


    //TODO: remove
    assert(as->segments[AS_NUM_SEG - 1].pbase == 0);
    as->segments[AS_NUM_SEG - 1].pbase = getppages(as->segments[AS_NUM_SEG - 1].size);
    if (as->segments[AS_NUM_SEG - 1].pbase == 0) {
        return ENOMEM;
    }


    //DEBUG(DB_ELF, "ELF: pbase: %x\n", as->segments[AS_NUM_SEG - 1].pbase);

    *stackptr = USERTOP;
    return 0;
}

struct segment * as_get_segment(struct addrspace * as, vaddr_t v) {

    int i = 0;
    for (i = 0; i < AS_NUM_SEG; i++) {
        if (v >= as->segments[i].vbase && v < as->segments[i].vbase + as->segments[i].size * PAGE_SIZE) {
            return &as->segments[i];
        }
    }
    return NULL;
}

int as_valid_read_addr(struct addrspace *as, vaddr_t *check_addr) {

    int i = 0;
    if (!(check_addr < (vaddr_t *) USERTOP)) {
        return 0;
    }

    for (i = 0; i < AS_NUM_SEG; i++) {
        if (as->segments[i].active) {
            if ((check_addr >= (vaddr_t *) as->segments[i].vbase) && (check_addr < (vaddr_t *) as->segments[i].vbase + PAGE_SIZE * as->segments[i].size)) {
                return 1;
            }
        }

    }
    return 0;
}

int as_valid_write_addr(struct addrspace *as, vaddr_t *check_addr) {

    int i = 0;
    if (!(check_addr < (vaddr_t *) USERTOP)) {
        return 0;
    }

    for (i = 0; i < AS_NUM_SEG; i++) {
        if (as->segments[i].active) {
            if ((check_addr >= (vaddr_t *) as->segments[i].vbase) && (check_addr < (vaddr_t *) as->segments[i].vbase + PAGE_SIZE * as->segments[i].size)) {
                return 1;
            }
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
#endif /* OPT_A2 */

#endif /* OPT_A3 */
