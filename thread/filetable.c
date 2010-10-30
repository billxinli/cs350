/*
 * Filetables for the thread. See filetable.h for details.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
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
    struct filetable *ft;
    ft = kmalloc(sizeof (struct filetable));
    if (ft == NULL) {
        return NULL;
    }
    ft->size = 0;
    ft->filedescriptor = array_create();
    if (ft->filedescriptor == NULL) {
        ft_destroy(ft);
        return NULL;
    }

    ft->nextfiledescriptor = q_create(100);
    if (ft->nextfiledescriptor == NULL) {
        ft_destroy(ft);
        return NULL;
    }


    return ft;
}

/*
 * ft_attachstds()
 * Attach the standard in, out, err to the file table, this can't be located the ft_create function
 * because that the device "con:" etc are not attached to the list of device when the os is the boot sequence
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
    fd_stdin->location = kstrdup("con:stdin");
    fd_stdin->mode = mode;
    fd_stdin->offset = 0;
    fd_stdin->fdvnode = vn_stdin;
    ft_add(ft, fd_stdin);

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
    fd_stdout->location = kstrdup("con:stdout");
    fd_stdout->mode = mode;
    fd_stdout->offset = 0;
    fd_stdout->fdvnode = vn_stdout;
    ft_add(ft, fd_stdout);

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
    fd_stderr->location = kstrdup("con:stderr");
    fd_stderr->mode = mode;
    fd_stderr->offset = 0;
    fd_stderr->fdvnode = vn_stderr;
    ft_add(ft, fd_stderr);

    return 1;
}

int ft_size(struct filetable *ft) {
    return (array_getnum(ft->filedescriptor));
}

struct filedescriptor *ft_get(struct filetable *ft, int fti) {


    if (ft_size(ft) == 0) {
        ft_attachstds(ft);
    }

    if (fti > ft_size(ft)) {
        return NULL;
    }
    struct filedescriptor *ret = NULL;
    ret = array_getguy(ft->filedescriptor, fti);
    return ret;
}

int ft_add(struct filetable* ft, struct filedescriptor* fd) {


    int fdn = 0;

    if (!q_empty(ft->nextfiledescriptor)) {
        fdn = (int) q_remhead(ft->nextfiledescriptor);

        fd->fdn = fdn;


        array_setguy(ft->filedescriptor, fdn, fd);
    } else {
        fdn = ft_size(ft);
        fd->fdn = fdn;
        array_add(ft->filedescriptor, fd);

    }


    return fdn;
}

int ft_remove(struct filetable* ft, int fti) {


    struct filedescriptor * fd;
    fd = ft_get(ft, fti);

    q_addtail(ft->nextfiledescriptor, (void *) fd->fdn);

    vfs_close(fd->fdvnode);

    if (fti == ft_size(ft)) {
        array_remove(ft->filedescriptor, fti);
    } else {
        array_setguy(ft->filedescriptor, fti, NULL);
    }



    return 1;
}

void ft_test(struct filetable* ft) {

    ft_attachstds(ft);


    kprintf("filetable test\n");
    int i = 0;
    for (i = 0; i < ft_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d\n", i);
        kprintf("fdn: %d   ", fd->fdn);
        kprintf("fdl: %s", fd->location);
        kprintf("\n");
    }
    ft_attachstds(ft);


    kprintf("filetable test2\n");

    for (i = 0; i < ft_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d\n", i);
        kprintf("fdn: %d   ", fd->fdn);
        kprintf("fdl: %s", fd->location);
        kprintf("\n");
    }

    ft_remove(ft, 4);
    ft_attachstds(ft);
    kprintf("filetable test2\n");
    for (i = 0; i < ft_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d                 ", i);
        kprintf("fdn: %d   ", fd->fdn);
        kprintf("fdl: %s", fd->location);
        kprintf("\n");
    }
    for (i = 0; i < 2147483647; i++) {
        kprintf("ft_size: %d\n", ft_size(ft));
        ft_attachstds(ft);
    }




    panic("test end");


}

int ft_destroy(struct filetable* ft) {
    (void) ft;
    return 1;
}

