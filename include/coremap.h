#include "opt-A3.h"
#if OPT_A3
#include <types.h>

#ifndef __COREMAP_H__
#define __COREMAP_H__

struct cm_detail;

struct cm_detail {
        int id;
        int kern; //indicate the page is a kernel page
        struct page_detail *pd;
        struct cm_detail *next_free;
        struct cm_detail *prev_free;
        int free;

};

struct cm {
        int init;
        int size;
        int clock_pointer;
        int lowest_frame;
        struct cm_detail *core_details;
        struct cm_detail *free_frame_list;
        struct cm_detail *last_free;
};

//Called to set up the core map
void cm_bootstrap();

//get the physical frame to copy our memory to
int cm_getppage();

//call to release a physical frame back into memory on program exit
void cm_release_frame(int frame_number);

//called by push to swap to get a frame ready for memory copy
void cm_free_core(struct cm_detail *cd, int spl);

//called after we have copied memory to our frame, tlb add should be called before
void cm_finish_paging(int frame, struct page_detail* pd);

int cm_push_to_swap();

vaddr_t cm_request_kframes(int num);

void cm_release_kframes(int frame_number);



#endif
#endif
