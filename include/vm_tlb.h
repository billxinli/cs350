#include "opt-A3.h"

#if OPT_A3
/*
struct tlbfreenode {
	int id;
	struct tlbfreenode *next;
};

struct tlbfreelist {
	struct tlbfreenode * tlbfreenodes;
	struct tlbfreenode * tlbnextfree;
};
*/
void tlb_bootstrap(void);
void tlb_add_entry(vaddr_t v, paddr_t p, int dirty);
void tlb_context_switch(void);
//private
int tlb_get_rr_victim(void);
int tlb_get_free_entry(void);

#endif /* OPT_A3 */
