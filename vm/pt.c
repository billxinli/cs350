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
#include <vmstats.h>
#include <coremap.h>

struct page_table* pt_create(struct segment* seg) {
    assert(seg != NULL);
    struct page_table * pt = kmalloc(sizeof (struct page_table));
    if (pt == NULL) {
        panic("Page table create failed. No more memory. \n");
        return NULL;
    }

    pt->size = seg->size;
    pt->seg = seg;
    pt->page_details = kmalloc(sizeof (struct page_detail) * pt->size);
    if (pt->page_details == NULL) {
        panic("Page table (page detail) create failed. No more memory. \n");
        return NULL;
    }
    
    int i;
    for (i = 0; i < pt->size; i++) {
            pt->page_details[i].vaddr = seg->vbase + PAGE_SIZE * i; //virtual page num
            pt->page_details[i].pfn = -1; //physical frame num
            pt->page_details[i].sfn = -1; //swap frame num
            pt->page_details[i].valid = 0; //valid bit
            pt->page_details[i].dirty = seg->writeable; //dirty bit (can we modify)
            pt->page_details[i].use = 0; //use bit (have we used the page recently)
    }
    
    return pt;
}

paddr_t pt_get_paddr(vaddr_t vaddr, struct segment *s) {
    //get the page detail
    int pt_offset_id = (vaddr - s->vbase) / PAGE_SIZE;
    struct page_detail *pd = &(s->pt->page_details[pt_offset_id]);
    if (pd->valid && pd->pfn != -1){
        pd->use = 1;
        int spl = splhigh();
        _vmstats_inc(VMSTAT_TLB_RELOAD);
        splx(spl);
    }else{
        paddr_t paddr = getppages(1);
        pd->pfn = paddr / PAGE_SIZE;
        pd->sfn = -1;
        pd->use = 1;
        load_segment_page(curthread->t_vmspace->file, vaddr, s, paddr);
        pd->valid = 1;
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
