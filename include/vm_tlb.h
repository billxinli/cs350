#include "opt-A3.h"
#ifndef _VM_TLB_H_
#define _VM_TLB_H_

#if OPT_A3

struct tlbfreenode {
	int id;
	struct tlbfreenode *next;
};

struct tlbfreelist {
	struct tlbfreenode * tlbfreenodes;
	struct tlbfreenode * tlbnextfree;
};

void tlb_bootstrap(void);
void tlb_context_switch(void);
void tlb_init_free_list(void);
void tlb_invalidate_vaddr(int vpn);
void tlb_add_entry(int vpn, paddr_t p, int dirty);

//private
int tlb_get_rr_victim(void);
int tlb_get_free_entry(void);

#endif /* OPT_A3 */
#endif
