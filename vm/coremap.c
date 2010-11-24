#include "opt-A3.h"

#if OPT_A3
#include <thread.h>
#include <vm.h>

struct cm;
struct cm_details;

void cm_bootstrap();
int cm_request_frame();
void cm_release_frame(int frame_number);

void cm_bootstrap(){
    core_map.size = mips_ramsize() / PAGE_SIZE;
    
    paddr_t physical_address = ram_stealmem(((sizeof(cm_details) * core_map.size) + PAGE_SIZE - 1) / PAGE_SIZE);
    if(physical_address == NULL){
        panic("NO MEMORY FOR CORE MAP");
    }
    core_details = (cm_details*) PADDR_TO_KVADDR(physical_address);
    
    paddr_t low;
    paddr_t high;
    ram_getsize(&low,&high);
    
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

int cm_request_frame(){
    if(core_map.free_frame_list == NULL){
        //free list is empty, run page replacement algorithm
        return cm_replace_page();
    }else{
        //get the frame from the free list
        return cm_get_free_frame();
    }
}

int cm_replace_page(){
    //run the clock algorithm
    while(true){
        //TODO
    }
}

int cm_get_free_frame(){
    int returnval = (int)(core_map.free_frame_list - core_map.core_details);
    core_map.free_frame_list = core_map.free_frame_list->next_free;
    core_map.core_details[returnval].next_free = NULL;
    return returnval;
}

int cm_invalidate_frame_page(int frame_number){
    //TODO
    return 0;
}

struct cm core_map;

struct cm{
    int size;
    int clock_pointer;
    int lowest_frame;
    struct cm_details* core_details;
    struct cm_details* free_frame_list = NULL;   
};

struct cm_details{
    int index;
    int vpn;                //virtual page num
    int kern                //indicate the page is a kernel page
    struct thread* program; //program owning page
    struct cm_details* next_free;
   
};
#endif /* OPT_A3 */
