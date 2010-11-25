#include <types.h>
#include <vnode.h>
#include <vmstats.h>
#include <uio.h>
#include <synch.h>
#include <vm.h>
#include <kern/unistd.h>
#include <fs.h>

//4 * 1024 * 1024 (we have a 4MB page file)
#define SWAP_SIZE = 4194304

///TODO: data type for pages
#define STRUCT_PAGE char[PAGE_SIZE];

struct vnode swapfile;
const int SWAP_PAGES = SWAP_SIZE / PAGE_SIZE;;
struct lock swapLock;

struct free_list {
    int swap_index_t;
    struct free_list *next;
};

struct free_list *freePages; //The index of the first free page
struct free_list *pageList; //link to the beginning of the array containing the indicies of free pages (not necessarily page that is actually free)

void create_swap() {
    lock_create(swapLock, "Swapfile Lock");
    _vmstats_init();
    paddr_t physical_address = ram_stealmem(((sizeof(free_list) * SWAP_PAGES) + PAGE_SIZE - 1) / PAGE_SIZE);
    freePages = (free_list *) PADDR_TO_KVADDR(physical_address);
    pageList = freePages;
    for (int i = 0; i < SWAP_PAGES; i++) {
        freePages[i].index = i;
        freePages[i].next = &freePages[i+1].next
    }
    freePages[SWAP_PAGES - 1].next = NULL: //fix the last element's next pointer
    //I'm not 100% confident I'm doing this exactly right, but I think this works
    struct vnode *root;
    vfs_lookup("/", root); 
    VOP_CREAT(root , "pagefile.sys", O_RDWR & O_CREAT, &swapfile);
    VOP_OPEN(swapfile, O_RDWR);
}

/*
returns 1 is the swap is full, 0 otherwise
*/
int swap_full() {
    return (freePages == NULL);
}

// frees a page for use in the swap file
void swap_free_page(int n) {
    lock_acquire(swapLock);
    pageList[n].next = freePages;
    freePages = pageList[n];`
    lock_release(swapLock);
}

void swap_write_page(STRUCT_PAGE *data, swap_index_t n) {
    mk_kuio(u, (void *) data, sizeof(STRUCT_PAGE), (int) n * sizeof(STRUCT_PAGE), UIO_WRITE);
    VOP_WRITE(swapfile, u);
}

/*
Writes a page to the swap file, and returns the index of the page in the swap file, which
is used by swap_read
*/
swap_index_t swap_write(STRUCT_PAGE *data) {
    swap_index_t pagenum;
    lock_acquire(swapLock);
    if (freePages == NULL) {
        panic("Out of memory!");
    } else {
        pagenum = freePages.index;
        freePages = freePages.next;
    }
    swap_write_page(data, pagenum);
    _vmstats_inc(VMSTAT_SWAP_FILE_WRITE);
    lock_release(swapLock);
    return pagenum;
}

/*
Loads a page from swapspace into RAM at the physical address specified by phys_addr
*/
void swap_read(paddr_t phys_addr, int n) {
    ///I'm not sure that I'm doing this right (specifically the PADDR_TO_KVADDR doesn't seem right)
    STRUCT_PAGE *page;
    lock_acquire(swapLock);
    mk_kuio(u, (void *) PADDR_TO_KVADDR(phys_addr) , sizeof(STRUCT_PAGE), n * sizeof(STRUCT_PAGE), UIO_READ);
    VOP_READ(swapfile, u);
    swap_free_page(n);
    _vmstats_inc(VMSTAT_SWAP_FILE_READ);
    lock_release(swapLock);
}