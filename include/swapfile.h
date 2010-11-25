#include "opt-A3.h"
#if OPT_A3

#ifndef __SWAP_FILE_H__
#define __SWAP_FILE_H__

#include <types.h>

void create_swap();
int swap_full();
void swap_free_page(int n);
int swap_write((void *) data); ///TODO: Is this the right data type
void swap_read(paddr_t phys_addr, int n);

#endif

#endif