/*
 * Filetables for the thread. See filetable.h for details.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <array.h>
#include <vfs.h>
#include <vnode.h>
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
        return NULL;
    }

    ft->size = 0;
    ft->filedescriptor = array_create();



    struct filedescriptor *fd;
    fd = kmalloc(sizeof (struct filedescriptor));
    if (fd == NULL) {
        ft_destroy(ft);
        return NULL;
    }

    struct vnode *v;
    v = kmalloc(sizeof(struct vnode));

    char *con;
    con = kstrdup("con");

    int mode = O_WRONLY;

    int result = vfs_open(con, mode, &v);
    kprintf("%d", result);


    /*

        struct vnode *vn;
        vn = kmalloc(sizeof(vn));

        char *con = kstrdup("con:");
        int res = vfs_open(con, O_WRONLY, &vn);

        kprintf("Result: %d\n",res);
     */
    return ft;
}

int ft_size(struct filetable *ft) {
    return ft->size;
}

struct filedescriptor *ft_get(struct filetable *ft, int fti) {
    return array_getguy(ft->filedescriptor, fti);
}

int ft_add(struct filetable* ft, struct filedescriptor* fdn) {
    (void) ft;
    (void) fdn;
    return 1;
}

int ft_remove(struct filetable* ft, int fti) {
    (void) ft;
    (void) fti;
    return 1;
}

int ft_destroy(struct filetable* ft) {
    (void) ft;
    return 1;
}




