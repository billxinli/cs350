#include "opt-A3.h"
#if OPT_A3
#ifndef _PT_H_
#define _PT_H_
#include <swapfile.h>
#include <addrspace.h>
#include <segments.h>
#include <thread.h>

struct page_detail {
	int seg_id;
	vaddr_t vaddr; //virtual page num
	int pfn; //physical frame num
	//TODO: fix
	int sfn; //swap frame num
	int valid; //valid bit
	int dirty; //dirty bit (can we modify)
	int use; //use bit (have we used the page recently)
};

//struct page_table;

struct page_table {
	int size;
	struct page_detail *page_details;
};


struct page_table* pt_create(struct segment *segments);
struct page_detail* pt_getpdetails(vaddr_t vaddr, struct addrspace * as);

paddr_t pt_get_paddr(struct addrspace *as, vaddr_t vaddr, struct segment *s);
void pt_destroy(struct page_table *pt);

#endif
#endif
