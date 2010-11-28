#include "opt-A3.h"
#if OPT_A3

#ifndef __COREMAP_H__
#define __COREMAP_H__

struct cm_details;

struct cm_details {
    int vpn;                //virtual page num
    int kern;                //indicate the page is a kernel page
    struct thread *program; //program owning page
    struct cm_details *next_free;
   
};

struct cm {
    int size;
    int clock_pointer;
    int lowest_frame;
    struct cm_details *core_details;
    struct cm_details *free_frame_list;
};

void cm_bootstrap();
int cm_request_frame();
void cm_release_frame(int frame_number);

#endif
#endif
