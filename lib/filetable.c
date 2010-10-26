/*
 * Filetables for the thread. See filetable.h for details.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <array.h>
#include <filetable.h>

// Structure of the filetable

struct filetable {
    //The table of file descriptors
    struct array *filedescriptor;
    //The size of the filetable
    int size;
};

// Structure of the filedescriptor

struct filedescriptor {
    //The mode of the file descriptor in question
    unsigned int mode;
    //The offset
    unsigned int offset;
    //The vnode
    struct vnode* vnode;
};

struct filetable *ft_create() {
    struct filetable *ft;
    ft = kmalloc(sizeof (struct filetable));
    if (ft == NULL) {
        //Panic?
        return NULL;
    }
    ft->size = 0;
    ft->filedescriptor = array_create();

    kprintf("filetable created.\n");

    return ft;

}

int ft_size(struct filetable *ft) {
    return ft->size;
}

struct filedescriptor *ft_get(struct filetable *ft, int fti) {
    return array_getguy(ft->filedescriptor, fti);
}

int ft_add(struct filetable* ft, filedescriptor* fdn) {
    return 1;
}

int ft_remove(struct filetable* ft, int fti) {
    return 1;
}

int ft_destroy(struct filetable* ft) {
    return 1;
}