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
#include <addrspace.h>
#include <vm_tlb.h>
#include <coremap.h>

struct page_table* pt_create(struct segment *segments) {
    struct page_table * pt = kmalloc(sizeof (struct page_table));
    if (pt == NULL) {
        panic("Page table create failed. No more memory. \n");
        return NULL;
    }

    int i = 0;
    size_t j = 0;
    int size = 0;

    for (i = 0; i < AS_NUM_SEG; i++) {
        size = size + segments[i].size;
    }
    pt->size = size;
    pt->page_details = kmalloc(sizeof (struct page_detail) *size);
    if (pt->page_details == NULL) {
        panic("Page table (page detail) create failed. No more memory. \n");
        return NULL;
    }

    int count = 0;
    for (i = 0; i < AS_NUM_SEG; i++) {
        for (j = 0; j < segments[i].size; j++) {
            pt->page_details[count].seg_id = i;
            pt->page_details[count].vaddr = (segments[i].vbase + PAGE_SIZE * j); //virtual page num
            pt->page_details[count].pfn = -1; //physical frame num
            pt->page_details[count].sfn = -1; //swap frame num
            pt->page_details[count].valid = 0; //valid bit
            pt->page_details[count].dirty = segments[i].writeable; //dirty bit (can we modify)
            pt->page_details[count].use = 0; //use bit (have we used the page recently)
        }
    }

    return pt;
}

struct page_detail* pt_getpdetails(vaddr_t vaddr, struct addrspace * as) {
    int i = 0;
    for (i = 0; i < as->pt->size; i++) {
        if (as->pt->page_details[i].vaddr == vaddr) {
            return &(as->pt->page_details[i]);
        }
    }
    return NULL;

}

paddr_t pt_get_paddr(struct addrspace *as, vaddr_t vaddr, struct segment *s) {
    struct page_detail *pd = pt_getpdetails(vaddr, as);
    DEBUG(DB_ELF, "PAGE_TABLE GET PADDR\n");
    if (pd->pfn == -1) {
        paddr_t paddr = getppages(1);
        pd->pfn = paddr / PAGE_SIZE;
        pd->sfn = -1;
        pd->valid = 1;
        pd->use = 1;
        load_segment_page(as->file, vaddr, s, paddr);
        DEBUG(DB_ELF, "PAGE_TABLE GET PPAGES: %x %x\n", paddr, pd->pfn*PAGE_SIZE);
    }
    return pd->pfn*PAGE_SIZE;
}

void pt_destroy(struct page_table * pt) {

    if (pt != NULL) {
        if (pt->page_details != NULL) {
            kfree(pt->page_details);
        }

        kfree(pt);
    }
}



#endif /* OPT_A3 */
