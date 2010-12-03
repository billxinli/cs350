#include "opt-A3.h"
#if OPT_A3
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <vm_tlb.h>
#include <vmstats.h>

struct tlbfreelist tlb_free_list;

void tlb_bootstrap(void) {
    tlb_free_list.tlbfreenodes = (struct tlbfreenode *) kmalloc(sizeof (struct tlbfreenode) * NUM_TLB);
    tlb_init_free_list();
}

void tlb_init_free_list(void) {
    int i;
    //set the free list to the first free node
    tlb_free_list.tlbnextfree = tlb_free_list.tlbfreenodes;

    //initialize the frame details
    for (i = 0; i < NUM_TLB - 1; i++) {
        tlb_free_list.tlbfreenodes[i].id = i;
        tlb_free_list.tlbfreenodes[i].next = &tlb_free_list.tlbfreenodes[i + 1];

    }

    tlb_free_list.tlbfreenodes[NUM_TLB - 1].id = NUM_TLB;
    tlb_free_list.tlbfreenodes[NUM_TLB - 1].next = NULL;
}

void tlb_add_entry(vaddr_t v, paddr_t p, int dirty, int update_stats) {
    int spl;

    assert((v & PAGE_FRAME) == v);
    assert((p & PAGE_FRAME) == p);

    spl = splhigh();
    int tlb_entry = tlb_get_free_entry();
    if (tlb_entry == -1) {
        if (update_stats) {
            _vmstats_inc(VMSTAT_TLB_FAULT_REPLACE);
        }
        tlb_entry = tlb_get_rr_victim();
    } else if (update_stats) {
        _vmstats_inc(VMSTAT_TLB_FAULT_FREE);
    }

    u_int32_t elo = p | TLBLO_VALID;
    if (dirty) {
        elo = elo | TLBLO_DIRTY;
    }

    DEBUG(DB_VMTLB, "dumbvm: 0x%x -> 0x%x numtlb: %d\n", v, p, tlb_entry);
    TLB_Write(v, elo, tlb_entry);
    splx(spl);
}

int tlb_get_rr_victim(void) {
    int victim;
    static unsigned int next_victim = 0;
    victim = next_victim;
    next_victim = (next_victim + 1) % NUM_TLB;
    return victim;
}

int tlb_get_free_entry(void) {
    if (tlb_free_list.tlbnextfree == NULL) {
        return -1;
    }
    int victim = tlb_free_list.tlbnextfree->id;

    tlb_free_list.tlbnextfree = tlb_free_list.tlbnextfree->next;
    tlb_free_list.tlbfreenodes[victim].next = NULL;

    return victim;
}

void tlb_invalidate_vaddr(vaddr_t v) {
    int victim;

    assert((v & PAGE_FRAME) == v);
    victim = TLB_Probe(v, 0);

    if (victim != -1) {
        TLB_Write(TLBHI_INVALID(victim), TLBLO_INVALID(), victim);
        tlb_free_list.tlbfreenodes[victim].next = tlb_free_list.tlbnextfree;
        tlb_free_list.tlbnextfree = &(tlb_free_list.tlbfreenodes[victim]);
    }
}

void tlb_context_switch(void) {
    int i, spl;

    spl = splhigh();
    tlb_init_free_list();

    _vmstats_inc(VMSTAT_TLB_INVALIDATE);
    for (i = 0; i < NUM_TLB; i++) {
        TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    }

    splx(spl);

}
#endif /* OPT_A3 */
