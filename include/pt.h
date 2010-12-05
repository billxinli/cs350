#include "opt-A3.h"
#if OPT_A3
#ifndef _PT_H_
#define _PT_H_
#include <swapfile.h>
#include <addrspace.h>
#include <segments.h>
#include <thread.h>

struct page_detail {
	vaddr_t vaddr; //virtual page num
	int pfn; //physical frame num
	//TODO: fix
	swap_index_t sfn; //swap frame num
	int valid; //valid bit
	int dirty; //dirty bit (can we modify)
	int use; //use bit (have we used the page recently)
};

//struct page_table;

struct page_table {
	int size;
	struct segment* seg;
	struct page_detail *page_details;
};


struct page_table* pt_create(struct segment *segments);

void pt_page_in(vaddr_t vaddr, struct segment *s);
void pt_destroy(struct page_table *pt);

#endif
#endif
