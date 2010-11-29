#include "opt-A3.h"
#if OPT_A3

#ifndef __COREMAP_H__
#define __COREMAP_H__

struct cm_detail;

struct cm_detail {
	int id;
	int vpn; //virtual page num
	int kern; //indicate the page is a kernel page
	struct thread *program; //program owning page
	struct cm_detail *next_free;

};

struct cm {
	int init;
	int size;
	int clock_pointer;
	int lowest_frame;
	struct cm_detail *core_details;
	struct cm_detail *free_frame_list;
};

void cm_bootstrap();
int cm_request_frame();
void cm_release_frame(int frame_number);
void cm_free_core(struct cm_detail *cd, struct page_detail * pd, int spl);

#endif
#endif
