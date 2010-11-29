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
    }
    core_map.core_details[0].next_free = NULL; //fix the last entry
}

void free_list_add(struct cm_detail *new) {
    if (core_map.free_frame_list == NULL) {
        core_map.free_frame_list = new;
        new->next_free = NULL;
    } else {
        new->next_free = core_map.free_frame_list;
        core_map.free_frame_list = new;
    }
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

    splx(spl);
    return retval;
}

int clock_to_index(int c) {
    return (c + core_map.lowest_frame);
}

int cm_push_to_swap() {
    int spl = splhigh();
    /**
    TODO: We need to make sure that the paging system knows that the page has been moved
    into swapspace and is no longer in RAM
     **/
    int i = 0;


    for (i = core_map.lowest_frame; i < core_map.size; i++) {
        ///TODO: first pass of page replacement algorithm
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
        ///TODO: second pass of page replacement algorithm
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
        ///TODO: third pass of page replacement algorithm

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
    assert(curspl > 0); ///Currently, interrupts must be disabled, but we should write this
    ///so that we use locks (will probably have to add another member to cm_details indicating
    ///if a frame is currently being moved to swap by another process
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

void cm_release_frame(int frame_number) {
    assert(curspl > 0); ///see note in cm_request frame about interrupts
    assert(frame_number >= core_map.lowest_frame);
    free_list_add(&core_map.core_details[frame_number]);
}

#endif /* OPT_A3 */
