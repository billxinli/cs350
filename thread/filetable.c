/*
 * Filetables for the thread. See filetable.h for details.
 */
#include "opt-A2.h"
#if OPT_A2
#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <machine/spl.h>
#include <lib.h>
#include <array.h>
#include <queue.h>
#include <vfs.h>
#include <vnode.h>
#include <filetable.h>
/*
 * ft_create()
 * Creates a file table that is attached to the thread library.
 * 
 */
struct filetable *ft_create() {
    //Allocate memory for the filetable structures
    struct filetable *ft = kmalloc(sizeof (struct filetable));
    if (ft == NULL) {
        return NULL;
    }
    ft->size = 0;
    //Allocate memory for the array of file descriptors
    ft->filedescriptor = array_create();
    if (ft->filedescriptor == NULL) {
        ft_destroy(ft);
        return NULL;
    }
    //preallocate the array to 20 files
    array_preallocate(ft->filedescriptor, 20);
    //Reserve the first three file descriptors for in out err.
    array_add(ft->filedescriptor, NULL);
    array_add(ft->filedescriptor, NULL);
    array_add(ft->filedescriptor, NULL);
    //Return the file table
    return ft;
}

/*
 * ft_attachstds()
 * Attach the standard in, out, err to the file table, this can't be located the
 * ft_create function because that the device "con:" etc are not attached to the
 * list of device when the os is the boot sequence.
 */
int ft_attachstds(struct filetable *ft) {
    char *console = NULL;
    int mode;
    int result = 0;
    //STDIN
    struct vnode *vn_stdin;
    mode = O_RDONLY;
    struct filedescriptor *fd_stdin = NULL;
    fd_stdin = (struct filedescriptor *) kmalloc(sizeof ( struct filedescriptor));
    if (fd_stdin == NULL) {
        ft_destroy(ft);
        return 0;
    }
    console = kstrdup("con:");
    result = vfs_open(console, mode, &vn_stdin);
    if (result) {
        vfs_close(vn_stdin);
        ft_destroy(ft);
        return 0;
    }
    kfree(console);
    fd_stdin->mode = mode;
    fd_stdin->offset = 0;
    fd_stdin->fdvnode = vn_stdin;
    fd_stdin->numOwners = 1;
    ft_set(ft, fd_stdin, STDIN_FILENO);
    //STDOUT
    struct vnode *vn_stdout;
    mode = O_WRONLY;
    struct filedescriptor *fd_stdout = NULL;
    fd_stdout = (struct filedescriptor *) kmalloc(sizeof (struct filedescriptor));
    if (fd_stdout == NULL) {
        ft_destroy(ft);
        return 0;
    }
    console = kstrdup("con:");
    result = vfs_open(console, mode, &vn_stdout);
    if (result) {
        vfs_close(vn_stdout);
        ft_destroy(ft);
        return 0;
    }
    kfree(console);
    fd_stdout->mode = mode;
    fd_stdout->offset = 0;
    fd_stdout->fdvnode = vn_stdout;
    fd_stdout->numOwners = 1;
    ft_set(ft, fd_stdout, STDOUT_FILENO);
    //STDERR
    struct vnode *vn_stderr;
    mode = O_WRONLY;
    struct filedescriptor *fd_stderr = NULL;
    fd_stderr = (struct filedescriptor *) kmalloc(sizeof (struct filedescriptor));
    if (fd_stderr == NULL) {
        ft_destroy(ft);
        return 0;
    }
    console = kstrdup("con:");
    result = vfs_open(console, mode, &vn_stderr);
    if (result) {
        vfs_close(vn_stderr);
        ft_destroy(ft);
        return 0;
    }
    kfree(console);
    fd_stderr->mode = mode;
    fd_stderr->offset = 0;
    fd_stderr->fdvnode = vn_stderr;
    fd_stderr->numOwners = 1;
    ft_set(ft, fd_stderr, STDERR_FILENO);
    return 1;
}

/*
 * ft_array_size()
 * This returns how big the array of the file descriptor is, this **DOES NOT**
 * tell you how many file descriptors are opened to the thread.
 */
int ft_array_size(struct filetable *ft) {
    assert(ft != NULL);
    return (array_getnum(ft->filedescriptor));
}

/*
 * ft_size()
 * This returns how many file descriptors are opened to the thread.
 */
int ft_size(struct filetable *ft) {
    assert(ft != NULL);
    int total = array_getnum(ft->filedescriptor);
    int i = 0;
    for (i = 0; i < ft_array_size(ft); i++) {
        if (ft_get(ft, i) == NULL) {
            total--;
        }
    }
    return total;
}

/*
 * ft_get()
 * This gets the file descriptor from a file table and the given file descriptor id.
 */
struct filedescriptor *ft_get(struct filetable *ft, int fti) {
    if (fti < 0) {
        return NULL;
    }
    //Since the stds are not attached to the thread, if the fd <=2 is requested,
    //then we will attach the stds to the thread.
    if (fti < 3) {
        if (array_getguy(ft->filedescriptor, fti) == NULL) {
            ft_attachstds(ft);
        }
    }
    //Doesn't exist.
    if (fti >= ft_array_size(ft)) { //changed > to >= since there shouldn't be an element ARRAY_SIZE --Matt
        return NULL;
    }
    struct filedescriptor *ret = array_getguy(ft->filedescriptor, fti);
    return ret;
}

int ft_set(struct filetable* ft, struct filedescriptor* fd, int fti) {
    if (fti >= ft_array_size(ft)) {
        return 1;
    }
    array_setguy(ft->filedescriptor, fti, fd);
    if (ft_get(ft, fti) == fd) {
        return 1;
    }
    return 0;
}

/*
 * ft_add()
 * This adds the file descriptor to the file table, it does by checking if there
 * is a free file descriptor in the queue, if not, it will add to the end of the
 * array. This will recover and reuse closed file descriptor ids.
 */
int ft_add(struct filetable* ft, struct filedescriptor* fd) {
    int fdn = 0;
    for (fdn = 0; fdn < ft_array_size(ft) && fdn < OPEN_MAX; fdn++) {
        if (ft_get(ft, fdn) == NULL) {
            array_setguy(ft->filedescriptor, fdn, fd);
            return fdn;
        }
    }
    if (fdn == OPEN_MAX) {
        return -1;
    }
    if (array_add(ft->filedescriptor, fd) != 0) { //if error (ENOMEM)
        return -1;
    }
    fd->numOwners++;
    assert(fdn != 0);
    return fdn;
}

/*
 * ft_remove()
 * This removes the file descriptor from the file table, it will add the file
 * descriptor id to the queue of availiable ids to use.
 */
int ft_remove(struct filetable* ft, int fti) {
    struct filedescriptor * fd = ft_get(ft, fti);
    if (fd != NULL) {
        //  q_addtail(ft->nextfiledescriptor, (void *) fd->fdn);
        int spl = splhigh();
        fd->numOwners--;
        if (fd->numOwners == 0) {
            vfs_close(fd->fdvnode);
            kfree(fd);
        }
        splx(spl);
        array_setguy(ft->filedescriptor, fti, NULL);
    }
    return 1;
}

/*
 * ft_destroy()
 * This will close and destroy all file descriptors in the file table. Should
 * only be called when the thread is exiting. In theory there shouldn't be
 * anything in here, except the stds, if they are attached to the thread.
 */
int ft_destroy(struct filetable* ft) {
    int i;
    for (i = ft_array_size(ft) - 1; i >= 0; i--) {
        ft_remove(ft, i);
    }
    kfree(ft);
    return 1;
}

/*
 * ft_test()
 * This tests the implementation of the file table, insertion, deletion, fd recovery.
 * This test is not passable, it will crash the kernel.
 */
void ft_test_list(struct filetable* ft) {
    kprintf("printing file descriptors\n");
    int i = 0;
    for (i = 0; i < ft_array_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d         ", i);

        if (fd != NULL) {
            kprintf("fdn: %d   ", fd->fdn);
        } else {
            kprintf("fdn: NULL   ");
        }
        kprintf("\n");
    }
}

/*
 * ft_test()
 * This tests the implementation of the file table, insertion, deletion, fd recovery.
 * This test is not passable, it will crash the kernel.
 */
void ft_test(struct filetable* ft) {
    kprintf("filetable test begin\n");
    kprintf("inserting file descriptors\n");
    ft_attachstds(ft);
    kprintf("printing file descriptors\n");
    int i = 0;
    for (i = 0; i < ft_array_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d\n", i);
        kprintf("fdn: %d   ", fd->fdn);
        kprintf("\n");
    }
    kprintf("inserting more file descriptors\n");
    ft_attachstds(ft);
    kprintf("printing file descriptors\n");
    for (i = 0; i < ft_array_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d ", i);
        kprintf("fdn: %d ", fd->fdn);
        kprintf("\n");
    }
    kprintf("removing file descriptor 4\n");
    ft_remove(ft, 4);
    kprintf("inserting more file descriptors\n");
    ft_attachstds(ft);
    kprintf("printing file descriptors\n");
    for (i = 0; i < ft_array_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d ", i);
        kprintf("fdn: %d ", fd->fdn);
        kprintf("\n");
    }
    kprintf("inserting infinite number of file descriptors\n");
    for (i = 0; i < 2147483647; i++) {
        kprintf("ft_array_size: %d\n", ft_array_size(ft));
        ft_attachstds(ft);
    }   
    panic("test end");
}

#endif
