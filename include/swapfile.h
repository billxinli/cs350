#include <types.h>

void create_swap();
void swap_free_page(int n);
int swap_write(/* page type */ *data);
void swap_read(paddr_t phys_addr, int n);