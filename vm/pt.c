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
#include <vmstats.h>
#include <coremap.h>
#include <vm_tlb.h>

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


void pt_page_in(vaddr_t vaddr, struct segment *s) {
    int spl = splhigh();
    _vmstats_inc(VMSTAT_TLB_FAULT);
    
    //get the page detail
    int pt_offset_id = (vaddr - s->vbase) / PAGE_SIZE;
    struct page_detail *pd = &(s->pt->page_details[pt_offset_id]);
    
    if (pd->valid && pd->pfn != -1){
        pd->use = 1;
        _vmstats_inc(VMSTAT_TLB_RELOAD);
        tlb_add_entry(vaddr, pd->pfn*PAGE_SIZE, s->writeable, 1); 
        splx(spl);
        return;
    }else if(pd->sfn != -1){
        splx(spl);
        pd->use = 1;
        pd->pfn = cm_getppage();
        swap_read(pd->pfn,pd->sfn);
        pd->sfn = -1;
        //add the tlb entry
        tlb_add_entry(vaddr, pd->pfn*PAGE_SIZE, s->writeable, 1);
        //finish the load
        cm_finish_paging(pd->pfn, pd);
    }else{
        splx(spl);
        pd->use = 1;
        pd->pfn = cm_getppage();
        load_segment_page(curthread->t_vmspace->file, vaddr, s, pd->pfn*PAGE_SIZE);
        //add the tlb entry
        tlb_add_entry(vaddr, pd->pfn*PAGE_SIZE, s->writeable, 1);
        //finish the load
        cm_finish_paging(pd->pfn, pd);
    }
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
