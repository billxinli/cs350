#include <vnode.h>
#include <vmstats.h>

struct vnode swapfile;

void create_swap() {
    /** sets up the swap file */
}

int swap_write(/* page type */ n) {
    /** writes data to the Nth page */
}

int swap_read(/* page type */ n) {
    /** seeks to the Nth page and reads it */
}