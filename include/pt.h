#include "opt-A3.h"
#if OPT_A3
#ifndef _PT_H_
#define _PT_H_
#include <swapfile.h>
#include <addrspace.h>

struct page_detail {
	int seg_id;
	int vpn; //virtual page num
	int pfn; //physical frame num
	swap_index_t sfn; //swap frame num
	int valid; //valid bit
	int dirty; //dirty bit (can we modify)
	int modified; //modified (have we modified)
	int use; //use bit (have we used the page recently)
};

//struct page_table;

struct page_table {
	int size;
	struct page_detail *page_details;
};


struct page_table* pt_create(struct segment *segments);
struct page_detail* pt_getpdetails(int vpn, struct thread * t);
int pt_checkreadonly(struct page_detail *pd);
void pt_loadpage(struct page_detail *pd, int faulttype);
void pt_destroy(struct page_table *pt) ;

#endif
#endif

