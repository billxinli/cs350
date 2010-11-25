#include "opt-A3.h"

#if OPT_A3
#include <thread.h>
#include <vm.h>
#include <swapfile.h>
#include <machine/spl.h>

struct cm_details {
    int index;
    int vpn;                //virtual page num
    int kern                //indicate the page is a kernel page
    struct thread* program; //program owning page
    struct cm_details* next_free;
   
};

struct cm {
    int size;
    int clock_pointer;
    int lowest_frame;
    struct cm_details* core_details;
    struct cm_details* free_frame_list = NULL;   
};

void cm_bootstrap();
int cm_request_frame();
void cm_release_frame(int frame_number);

/// move the above to header file, ifndef block, blah blah blah
////////////////////////////////////////////////

struct cm core_map;

void cm_bootstrap(){
    assert(curspl>0);
    assert(core_map == NULL); //we had better not bootstrap more than once!
    
    core_map.size = mips_ramsize() / PAGE_SIZE;
    
    paddr_t physical_address = ram_stealmem(((sizeof(cm_details) * core_map.size) + PAGE_SIZE - 1) / PAGE_SIZE);
    if(physical_address == NULL){
        panic("NO MEMORY FOR CORE MAP");
    }
    core_details = (cm_details*) PADDR_TO_KVADDR(physical_address);
    
    paddr_t low;
    paddr_t high;
    ram_getsize(&low,&high);
    assert(high != 0); //if this assertion fails, ram_getsize has been called before
    
    core_map.lowest_frame = (low / PAGE_SIZE);
    
    //set the free list to the first frame
    core_map.free_frame_list = &(core_map.cm_details[core_map.lowest_frame]);
    //initialize the frame details
    int i;
    for(i = core_map.lowest_frame; i < (core_map.size - 1); i++){
        core_map.cm_details[i].vpn = -1;
        core_map.cm_details[i].kern = 0;
        core_map.cm_details[i].program = NULL;
        core_map.cm_details[i].next_free = &(core_map.cm_details[i+1]);
    }
    
    core_map.cm_details[core_map.size - 1].vpn = -1;
    core_map.cm_details[core_map.size - 1].kern = 0;
    core_map.cm_details[core_map.size - 1].program = NULL;
    core_map.cm_details[core_map.size - 1].next_free = NULL;
}

void free_list_add(struct cm_details *new) {
    if (core_map.free_frame_list == NULL) {
        core_map.free_frame_list = new;
        new->next_free = NULL;
    } else {
        new->next_free = core_map.free_frame_list;
        core_map.free_frame_list = new;
    }
}

struct cm_details *free_list_pop() {
    if (core_map.free_frame_list == NULL) {
        return NULL;
    }
    struct cm_details *retval;
    retval = core_map.free_frame_list;
    core_map.free_frame_list = core_map.free_frame_list->next_free;
    return retval;
}

int cm_push_to_swap() {
    /**
    TODO: We need to make sure that the paging system knows that the page has been moved
    into swapspace and is no longer in RAM
    **/
    for (int i = core_map.lowest_frame; i < core_map.size; i++) {
        ///TODO: first pass of page replacement algorithm
    }
    for (int i = core_map.lowest_frame; i < core_map.size; i++) {
        ///TODO: second of page replacement algorithm- should return before loop exits
    }
    //if we get here, then all pages are kernel pages (which must remain in RAM), so we're out of memory. That sucks!
    panic("Too many kernel pages! No more space in RAM for a new page.");
}

int cm_request_frame(){
    assert(curspl>0); ///Currently, interrupts must be disabled, but we should write this
    ///so that we use locks (will probably have to add another member to cm_details indicating
    ///if a frame is currently being moved to swap by another process
    struct cm_details *frame = free_list_pop();
    if (frame == NULL) {
        /*
        if no free pages are available, we need to push a frame into
        swap to make room for a new frame in RAM
        */
        return cm_push_to_swap();
    } else {
        //there is a free page, so return it's index
        return (((int) frame - (int) core_map.core_details) / sizeof(struct cm_details));
    }
}

void cm_release_frame(int frame_number){
    assert(curspl>0); ///see note in cm_request frame about interrupts
    assert(frame_number >= core_map.lowest_frame);
    free_list_add(&core_map.core_details[frame_number]);
}

#endif /* OPT_A3 */
