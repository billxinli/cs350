#include "opt-A2.h"
#if OPT_A2
#ifndef _FILETABLE_H_
#define _FILETABLE_H_

/*
 * Filetables
 * Filetables belongs to threads, and is responsible for the file descriptors (fd)
 * that are availiable to each of the threads.
 * Initially each thread have three fds, stdin, stdout, and stderr.
 * Filetables is required for operations that read, write operations,
 * such as write to terminal, write to file, open file, etc.
 *
 * Functions:
 *     ft_create  - allocate a new filetable object. Returns NULL if out of memory.
 *     ft_array_size    - returns the size of the filetable.
 *     ft_get     - returns the fti th filedescriptor from the filetable.
 *     ft_add     - add a filedescriptor to the filetable.
 *     ft_remove  - remove the fti th filedescriptor from the filetable.
 *     ft_destroy - destroy the filetable.
 *     ft_test    - tests the implementation of the filetable, will crash the kernel.
 */

// Structure of the filedescriptor
struct filedescriptor;

struct filedescriptor {
	//The file descriptor number
	int fdn;
	//The mode of the file descriptor in question
	int mode;
	//Number of owners of the file descriptor
	int numOwners;
	//The offset
	int offset;
	//The vnode
	struct vnode* fdvnode;
};

// Structure of the filetable
struct filetable;

struct filetable {
	//The table of file descriptors
	struct array *filedescriptor;
	//The size of the filetable
	int size;
};

struct filetable *ft_create();
int ft_attachstds(struct filetable *ft);
int ft_array_size(struct filetable *ft);
int ft_size(struct filetable *ft);
struct filedescriptor *ft_get(struct filetable *ft, int fti);
int ft_set(struct filetable* ft, struct filedescriptor* fdn, int fti);
int ft_add(struct filetable* ft, struct filedescriptor* fdn);
int ft_remove(struct filetable* ft, int fti);
int ft_destroy(struct filetable* ft);
void ft_test_list(struct filetable* ft);
void ft_test(struct filetable* ft);
#endif /* _FILETABLE_H_ */
#endif /* OPT_A2 */
