#include "opt-A3.h"

#if OPT_A3

#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <vmstats.h>
#include <uio.h>
#include <synch.h>
#include <vm.h>
#include <kern/unistd.h>
#include <fs.h>
#include <swapfile.h>
#include <lib.h>

//4 * 1024 * 1024 (we have a 4MB page file)
#define SWAP_SIZE 4194304

struct vnode *swapfile;
int SWAP_PAGES = SWAP_SIZE / PAGE_SIZE;

struct lock *swapLock;

struct free_list {
    swap_index_t index;
    struct free_list *next;
};

struct free_list *freePages; //The index of the first free page
struct free_list *pageList; //link to the beginning of the array containing the indicies of free pages (not necessarily page that is actually free)

/*
Creates a swapspace file for use by the operating system. May only be called once
 */
void create_swap() {
    swapLock = lock_create_nokmalloc("Swapfile Lock");
    _vmstats_init();
    freePages = (struct free_list *) ralloc((int) sizeof(struct free_list));
    pageList = freePages;
    int i = 0;
    for (i = 0; i < SWAP_PAGES; i++) {
        freePages[i].index = i;
        freePages[i].next = &freePages[i + 1];
    }
    freePages[SWAP_PAGES - 1].next = NULL; //fix the last element's next pointer
    //I'm not 100% confident I'm doing this exactly right, but I think this works
    struct vnode *root;
    vfs_lookup("/", &root);
    VOP_CREAT(root, "SWAPFILE", 1, &swapfile);
    VOP_OPEN(swapfile, O_RDWR);
}

/*
Checks to see if the swap file is full. Returns 1 if all pages are used and 0 otherwise
 */
int swap_full() {
    return (freePages == NULL);
}

/*
Frees a page in the swap file for reuse (but does not zero it)
 */
void swap_free_page(swap_index_t n) {
    lock_acquire(swapLock);
    pageList[(int) n].next = freePages;
    freePages = &pageList[(int) n];
    lock_release(swapLock);
}

void swap_write_page(void *data, swap_index_t n) {
    struct uio *u = kmalloc(sizeof (struct uio));
    mk_kuio(u, data, PAGE_SIZE, (int) n * PAGE_SIZE, UIO_WRITE);
    VOP_WRITE(swapfile, u);
}

/*
Writes data to a free page in the swapfile and returns the index of the page
in the swapfile
 */
swap_index_t swap_write(void *data) {
    swap_index_t pagenum;
    lock_acquire(swapLock);
    if (freePages == NULL) {
        panic("Out of memory!");
    } else {
        pagenum = freePages->index;
        freePages = freePages->next;
    }
    swap_write_page(data, pagenum);
    _vmstats_inc(VMSTAT_SWAP_FILE_WRITE);
    lock_release(swapLock);
    return pagenum;
}

/*
Reads the page at index n in the swapfile into memory at physical address phys_addr
 */
void swap_read(paddr_t phys_addr, swap_index_t n) {
    ///I'm not sure that I'm doing this right (specifically the PADDR_TO_KVADDR doesn't seem right)
    lock_acquire(swapLock);
    struct uio *u = kmalloc(sizeof (struct uio));
    mk_kuio(u, (void *) PADDR_TO_KVADDR(phys_addr), PAGE_SIZE, (int) n * PAGE_SIZE, UIO_READ);
    VOP_READ(swapfile, u);
    swap_free_page(n);
    _vmstats_inc(VMSTAT_SWAP_FILE_READ);
    lock_release(swapLock);
}

#endif
