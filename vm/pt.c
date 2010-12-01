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

struct page_table* pt_create(struct segment *segments) {
    struct page_table * pt = kmalloc(sizeof (struct page_table));
    if (pt == NULL) {
        panic("Page table create failed. No more memory. \n");
        return NULL;
    }

    int i = 0;
    int j = 0;
    int size = 0;

    for (i = 0; i < 3; i++) {

        size = size + segments[i].size;

    }
    pt->page_details = kmalloc(sizeof (struct page_detail) *size);
    if (pt->page_details == NULL) {
        panic("Page table (page detail) create failed. No more memory. \n");
        return NULL;
    }

    int count = 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < segments[i].size; j++) {
            pt->page_details[count].seg_id = i;
            pt->page_details[count].vpn = (segments[i].vbase + PAGE_SIZE * j) / PAGE_SIZE; //virtual page num
            pt->page_details[count].pfn = -1; //physical frame num
            pt->page_details[count].sfn = -1; //swap frame num
            pt->page_details[count].valid = 0; //valid bit
            pt->page_details[count].dirty = segments[i].writeable; //dirty bit (can we modify)
            pt->page_details[count].modified = 0; //modified (have we modified)
            pt->page_details[count].use = 0; //use bit (have we used the page recently)
        }
    }

    return pt;
}

struct page_detail* pt_getpdetails(int vpn, struct thread * t) {
    int i = 0;
    for (i = 0; i < t->pt->size; i++) {
        if (t->pt->page_details[i].vpn == vpn) {
            return &(t->pt->page_details[i]);
        }
    }
    return NULL;

}

int pt_checkreadonly(struct page_detail *pd) {
    if (pd->writeable) {
        pd->modified = 1;
        tlb_invalidate_vaddr(pd->vpn);
        tlb_add_entry(pd->vpn, pd->pfn, 1);
        return 0;
    }
    return 1;


}

void pt_loadpage(struct page_detail *pd, int faulttype) {
    int frame = cm_request_frame();
    if (pd->sfn == -1) {

    } else {
        swap_read(frame, pd->sfn);
    }

    pd->pfn = frame;
    pd->sfn = -1;
    pd->valid = 1;
    pd->modified = 0;
    pd->use = 1;

}

void pt_destroy(struct page_table *pt) {

    if (pt != NULL) {
        if (pt->page_details != NULL) {
            kfree(pt->page_details);
        }

        kfree(pt);
    }
}



#endif /* OPT_A3 */
