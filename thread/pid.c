/*
This file contains the methods used for assigning process IDs to new threads and
freeing these IDs for re-use when the thread exits. These operations are atomic.
*/

#include "opt-A2.h"
#if OPT_A2

#include <lib.h>
#include <types.h>
#include <pid.h>
#include <machine/spl.h>

struct pid_list {
    pid_t pid;
    struct pid_list *next;
}

unsigned int unused_pids = 1;
struct pid_list *recycled_pids = NULL;
struct pid_list *last_recycled_pid = NULL;

/*
Find a pid for a new process
*/
pid_t new_pid() {
    int spl = splhigh();
    if (recycled_pids == NULL) {
        assert(unused_pids < 0x7FFFFFFF); //can't even happen with sys161's available memory
        unused_pids += 1;
        splx(spl);
        return (unused_pids - 1);
    } else {
        struct pid_list *first = recycled_pids;
        recycled_pids = recycled_pids->next;
        pid_t ret = first->pid;
        kfree(first);
        splx(spl);
        return ret;
    }
}

/*
Frees a pid for re-use
*/
void reclaim_pid(pid_t x) {
    int spl = splhigh();
    if (recycled_pids == NULL) {
        recycled_pids = kmalloc(sizeof(pid_list));
        if (recycled_pids == NULL) {
            panic("Out of memory!");
        }
        recycled_pids->pid = x;
        recycled_pids->next = NULL;
        last_recycled_pid = recycled_pids;
    } else {
        last_recycled_pid->next = kmalloc(sizeof(pid_list));
        last_recycled_pid = last_recycled_pid->next;
        if (last_recycled_pid == NULL) {
            panic("Out of memory!");
        }
        last_recycled_pid->pid = x;
        last_recycled_pid->next = NULL;
    }
    splx(spl);
}

#endif
