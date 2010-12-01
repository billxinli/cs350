#include "opt-A3.h"

#if OPT_A3
#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <machine/spl.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <thread.h>
#include <vm.h>
#include <swapfile.h>
#include <coremap.h>
#include <pt.h>
#include <vm_tlb.h>


struct cm core_map;

void cm_bootstrap() {
    assert(curspl > 0);
    assert(core_map.init == 0); //we had better not bootstrap more than once!
    core_map.init = 1;
    core_map.size = mips_ramsize() / PAGE_SIZE;

    core_map.core_details = (struct cm_detail*) ralloc(sizeof (struct cm_detail) * core_map.size);

    paddr_t low;
    paddr_t high;
    ram_getsize(&low, &high);
    assert(high != 0); //if this assertion fails, ram_getsize has been called before

    core_map.lowest_frame = (low / PAGE_SIZE);

    //set the free list to the first frame
    core_map.free_frame_list = &(core_map.core_details[core_map.lowest_frame]);
    //initialize the frame details
    int i;
    for (i = core_map.size - 1; i >= core_map.lowest_frame; i--) {
        core_map.core_details[i].id = i;
        core_map.core_details[i].vpn = -1;
        core_map.core_details[i].kern = 0;
        core_map.core_details[i].program = NULL;
        core_map.core_details[i].next_free = &(core_map.core_details[i - 1]);
        core_map.core_details[i].prev_free = &(core_map.core_details[i + 1]);
        core_map.core_details[i].free = 1;
    }
    core_map.core_details[0].next_free = NULL; //fix the last entry
    core_map.core_details[core_map.size - 1].prev_free = NULL; //fix the first entry
}

void free_list_add(struct cm_detail *new) {
    int spl = splhigh();
    if (core_map.free_frame_list == NULL) {
        core_map.free_frame_list = new;
        new->next_free = NULL;
        new->prev_free = NULL;
    } else {
        new->next_free = core_map.free_frame_list;
        new->prev_free = NULL;
        core_map.free_frame_list->prev_free = new;
        core_map.free_frame_list = new;
    }
    new->free = 1;
    splx(spl);
}

struct cm_detail *free_list_pop() {
    int spl = splhigh();

    if (core_map.free_frame_list == NULL) {
        splx(spl);
        return NULL;
    }
    struct cm_detail *retval;
    retval = core_map.free_frame_list;
    core_map.free_frame_list = core_map.free_frame_list->next_free;
    core_map.free_frame_list->prev_free = NULL;
    /*
      temporarily set to a kernel page, so that it can't be swapped out
      until we finish the cm_request_frame call
    */
    retval->kern = 1;
    retval->free = 0;

    splx(spl);
    return retval;
}

int clock_to_index(int c) {
    return (c + core_map.lowest_frame);
}

int cm_push_to_swap() {
    int spl = splhigh();
    int i = 0;


    for (i = core_map.lowest_frame; i < core_map.size; i++) {
        
        struct cm_detail *cd = &core_map.core_details[clock_to_index(core_map.clock_pointer)];
        if (cd->kern == 1) {
            struct page_detail * pd = pt_getpdetails(cd->vpn, cd->program);

            if (pd == NULL) {
                panic("Page table missing details for VPN: %d\n", cd->vpn);
            }
            if (pd->use == 0 && pd->modified == 0) {
                cm_free_core(cd, pd, spl);
                return cd->id;
            }
        }
        core_map.clock_pointer = (core_map.clock_pointer + 1) % (core_map.size - core_map.lowest_frame);

    }
    for (i = core_map.lowest_frame; i < core_map.size; i++) {
        
        struct cm_detail *cd = &core_map.core_details[clock_to_index(core_map.clock_pointer)];
        if (cd->kern == 1) {
            struct page_detail * pd = pt_getpdetails(cd->vpn, cd->program);
            if (pd == NULL) {
                panic("Page table missing details for VPN: %d\n", cd->vpn);
            }
            if (pd->use == 0) {
                cm_free_core(cd, pd, spl);
                return cd->id;
            } else {
                pd->use = 0;
                tlb_invalidate_vaddr(pd->vpn);
            }
        }
        core_map.clock_pointer = (core_map.clock_pointer + 1) % (core_map.size - core_map.lowest_frame);
    }
    for (i = core_map.lowest_frame; i < core_map.size; i++) {
        

        struct cm_detail *cd = &core_map.core_details[clock_to_index(core_map.clock_pointer)];
        if (cd->kern == 1) {
            struct page_detail * pd = pt_getpdetails(cd->vpn, cd->program);

            if (pd == NULL) {
                panic("Page table missing details for VPN: %d\n", cd->vpn);
            }
            cm_free_core(cd, pd, spl);
            return cd->id;

        }
        core_map.clock_pointer = (core_map.clock_pointer + 1) % (core_map.size - core_map.lowest_frame);
    }
    splx(spl);
    //if we get here, then all pages are kernel pages (which must remain in RAM), so we're out of memory. That sucks!
    panic("Too many kernel pages! No more space in RAM for a new page.");
    return 0;
}

void cm_free_core(struct cm_detail *cd, struct page_detail * pd, int spl) {


    pd->valid = 0;
    cd->kern = 1;
    splx(spl);

    pd->sfn = swap_write(cd->id);

    cd->vpn = -1;
    cd->program = NULL;
    cd->next_free = NULL;
}

int cm_request_frame() {
    struct cm_detail *frame = free_list_pop();
    if (frame == NULL) {
        /*
        if no free pages are available, we need to push a frame into
        swap to make room for a new frame in RAM
         */
        return cm_push_to_swap();
    } else {
        //there is a free page, so return it's index
        return (((int) frame - (int) core_map.core_details) / sizeof (struct cm_detail));
    }
}

int kalloced(struct cm_detail *frame) {
    return (frame->kern && frame->free == 0);
}

vaddr_t cm_request_kframes(int num) {
    assert(core_map.init); //don't call kmalloc before coremap is setup
    assert(curspl == 0); //interrupts must be off when calling kmalloc
    int spl = splhigh();
    int frame = -1;
    int i; int j;
    for (i = core_map.lowest_frame; i < core_map.size; i++) {
        frame = i;
        for (j = 0; j < num; j++) {
            if (kalloced(&core_map.core_details[i+j])) {
                frame = -1;
                break;
            }
        }
        if (frame != -1) break;
    }
    if (frame == -1) {
        panic("No space to get %d kernel page(s)!", num);
    }
    /*
    NOTE: Instead of using more memory to store another variable in cm_details,
    since vpn is unused for kernel pages, we recycle the vpn value so that in the
    first page of a series of continuous kernel pages, the vpn is the number of
    pages allocated for the large allocation. This is used to know how many
    pages to free when we call cm_release_kframes
    */
    core_map.core_details[frame].vpn = num;
    // ///////////////
    for (i = frame; i < frame + num; i++) {
        assert(core_map.core_details[i].free || core_map.core_details[i].kern == 0);
        core_map.core_details[i].kern = 1;
    }

    for (i = frame; i < frame + num; i++) {
        if (core_map.core_details[i].free) {
            if (core_map.free_frame_list == &core_map.core_details[i]) {
                core_map.free_frame_list->next_free->prev_free = NULL;
                core_map.free_frame_list = core_map.free_frame_list->next_free;
            } else {
                core_map.core_details[i].prev_free->next_free = core_map.core_details[i].next_free;
                core_map.core_details[i].next_free->prev_free = core_map.core_details[i].prev_free;
            }
            core_map.core_details[i].free = 0;
        } else {
            cm_free_core(&core_map.core_details[i],pt_getpdetails(core_map.core_details[i].vpn, core_map.core_details[i].program) ,spl);
            spl = splhigh();
        }
        
    }
    splx(spl);
    return PADDR_TO_KVADDR((paddr_t) (frame * PAGE_SIZE));
}

void cm_release_frame(int frame_number) {
    assert(frame_number >= core_map.lowest_frame);
    free_list_add(&core_map.core_details[frame_number]);
}

void cm_release_kframes(int frame_number) {
    int num = core_map.core_details[frame_number].vpn; //see note above; in kernel pages, it stores the number of linked continuous pages
    assert(num > 0);
    int i;
    for (i = frame_number; i < frame_number + num; i++) {
        assert(kalloced(&core_map.core_details[i]));
        cm_release_frame(i);
    }
}
//call this after cm_request_frame (but not after cm_request_kframe)
void cm_done_request(int frame) {
    core_map.core_details[frame].kern = 0;
}

#endif /* OPT_A3 */
