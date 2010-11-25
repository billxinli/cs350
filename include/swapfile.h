#include "opt-A3.h"
#if OPT_A3

#ifndef __SWAP_FILE_H__
#define __SWAP_FILE_H__

#include <types.h>
#include <vm.h>

typedef int swap_index_t;

void create_swap();
int swap_full();
void swap_free_page(int n);
swap_index_t swap_write(char[PAGE_SIZE]);
void swap_read(paddr_t phys_addr, int n);

#endif

#endif