#include <types.h>
#include <vnode.h>
#include <vmstats.h>
#include <uio.h>
#include <synch.h>

#define SWAP_SIZE = 4194304; //4 * 1024 * 1024

struct vnode swapfile;
int SWAP_PAGES;
struct lock swapLock;

void create_swap() {
    //TODO
    SWAP_PAGES = SWAP_SIZE / PAGE_SIZE;
    lock_create(swapLock, "Swapfile Lock");
    _vmstats_init();
    /** sets up the swap file */
    /** sets up any structures needed for keeping track of used pages **/
}

void swap_free_page(int n) {
    //TODO
    lock_acquire(swapLock);
    /** Frees a page from the swap file */
    lock_release(swapLock);
}

void swap_write_page(/* page type */ *data, int n) {
    mk_kuio(u, (void *) data, sizeof(/* page type */), n * sizeof(/* page type */), UIO_WRITE);
    VOP_WRITE(swapfile, u);
}

/*
Writes a page to the swap file, and returns the index of the page in the swap file, which
is used by swap_read
*/
int swap_write(/* page type */ *data) {
    //TODO
    int pagenum;
    lock_acquire(swapLock);
    ///TODO: Find a free page in the swap file (if there is one)
    swap_write_page(data, pagenum);
    _vmstats_inc(VMSTAT_SWAP_FILE_WRITE);
    lock_release(swapLock);
}

/*
Loads a page from swapspace into RAM at the physical address specified by phys_addr
*/
void swap_read(paddr_t phys_addr, int n) {
    ///I'm not sure that I'm doing this right (specifically the PADDR_TO_KVADDR doesn't seem right)
    /* page type */ *page;
    lock_acquire(swapLock);
    mk_kuio(u, (void *) PADDR_TO_KVADDR(phys_addr) , sizeof(/* page type */), n * sizeof(/* page type */), UIO_READ);
    VOP_READ(swapfile, u);
    swap_free_page(n);
    _vmstats_inc(VMSTAT_SWAP_FILE_READ);
    lock_release(swapLock);
}