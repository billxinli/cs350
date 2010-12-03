#include "opt-A3.h"
#if OPT_A3

#ifndef __SWAPFILE_H__
#define __SWAPFILE_H__

#include <types.h>
#include <vm.h>

typedef  int swap_index_t;

/*
Creates a swapspace file for use by the operating system. May only be called once
*/
void swap_bootstrap();

/*
Checks to see if the swap file is full. Returns 1 if all pages are used and 0 otherwise
*/
int swap_full();

/*
Frees a page in the swap file for reuse (but does not zero it)
*/
void swap_free_page(swap_index_t n);

/*
Writes data to a free page in the swapfile and returns the index of the page
in the swapfile (pass the physical frame number)
*/
swap_index_t swap_write(int phys_frame_num);

/*
Reads the page at index n in the swapfile into memory at the specified physical
frame
*/
void swap_read(int phys_frame_num, swap_index_t n);

#endif

#endif
