/*
This file contains the methods used for assigning process IDs to new threads and
freeing these IDs for re-use when the thread exits. These operations are atomic.
*/

#include "opt-A2.h"
#if OPT_A2

#include <types.h>
#include <lib.h>
#include <types.h>
#include <pid.h>
#include <machine/spl.h>

#define PID_FREE   0 //the process can be recycled
#define PID_PARENT 1 //the process has not exited, but the parent has exited
#define PID_EXITED 2 //the process has exited, but the parent has not exited or waited on this pid
#define PID_NEW    3 //neither the process nor its parent have exited

struct pid_clist {
    pid_t pid;
    int status;
    struct pid_clist *next;
};

struct pid_list {
    pid_t pid;
    struct pid_list *next;
};

unsigned int unused_pids = MIN_PID;
struct pid_list *recycled_pids;
struct pid_clist *unavailable_pids;

/*
Find a pid for a new process
*/
pid_t new_pid() {
    int spl = splhigh();
    if (recycled_pids == NULL) {
        assert(unused_pids < 0x7FFFFFFF); //can't even happen with sys161's available memory
        struct pid_clist *new_entry = kmalloc(sizeof(struct pid_clist));
        new_entry->pid = unused_pids;
        new_entry->status = PID_NEW;
        new_entry->next = unavailable_pids;
        unavailable_pids = new_entry;
        unused_pids += 1;
        splx(spl);
        DEBUG(DB_PID, "A thread has been assigned PID #%d\n", unused_pids-1);
        return (unused_pids - 1);
    } else {
        struct pid_list *first = recycled_pids;
        recycled_pids = recycled_pids->next;
        struct pid_clist *new_entry = kmalloc(sizeof(struct pid_clist));
        new_entry->pid = first->pid;
        new_entry->status = PID_NEW;
        new_entry->next = unavailable_pids;
        unavailable_pids = new_entry;
        kfree(first);
        splx(spl);
        DEBUG(DB_PID, "A thread has been assigned PID #%d\n", (int) new_entry->pid);
        return (new_entry->pid);
    }
}

//"private" function
void pid_change_status(pid_t x, int and_mask) {
    int spl = splhigh();
    DEBUG(DB_PID, "PID #%d: change status %d\n", (int) x, and_mask);
    assert(unavailable_pids != NULL);
    if (unavailable_pids->pid == x) {
        unavailable_pids->status &= and_mask;
        DEBUG(DB_PID, "PID #%d: new status is %d\n", (int) x, unavailable_pids->status);
        if (unavailable_pids->status == PID_FREE) {
            //add pid to recycled_pids list
            struct pid_list *new_entry = kmalloc(sizeof(struct pid_list));
            new_entry->pid = x;
            new_entry->next = recycled_pids;
            recycled_pids = new_entry;
            //remove pid from unavailable_pids list
            struct pid_clist *temp = unavailable_pids;
            unavailable_pids = unavailable_pids->next;
            kfree(temp);
            DEBUG(DB_PID, "PID #%d: free for re-use\n", (int) x);
        }
    } else {
        int found = 0;
        struct pid_clist *p = unavailable_pids;
        while (p->next != NULL) {
            if (p->next->pid == x) {
                found = 1;
                p->next->status &= and_mask;
                DEBUG(DB_PID, "PID #%d: new status is %d\n", (int) x, p->next->status);
                if (p->next->status == PID_FREE) {
                    //add pid to recycled_pids list
                    struct pid_list *new_entry = kmalloc(sizeof(struct pid_list));
                    new_entry->pid = x;
                    new_entry->next = recycled_pids;
                    recycled_pids = new_entry;
                    //remove pid from unavailable_pids list
                    struct pid_clist *temp = p->next;
                    p->next = p->next->next;
                    kfree(temp);
                    DEBUG(DB_PID, "PID #%d: free for re-use\n", (int) x);
                }
            }
            p = p->next;
        }
        assert(found);
    }
    splx(spl);
}

/*
Changes a pid's status to indicate that process's parent has exited, so the pid
does not need to be saved after the process exits. Frees the pid it if can be recycled
*/
void pid_parent_done(pid_t x) {
    pid_change_status(x, PID_PARENT);
}

/*
Changes a pid's status to indicate that the process with that pid has closed,
freeing it if it can be recycled
*/
void pid_process_exit(pid_t x) {
    pid_change_status(x, PID_EXITED);
}

/*
recycles the pid; used when a thread with no parent exits
*/
void pid_free(pid_t x) {
    pid_change_status(x, PID_FREE);
}


/*
checks weather or not a PID is in use. This is only used when an invalid PID
is used in waitpid to determine weather we should give error EINVAL or ESRCH
*/
int pid_claimed(pid_t x) {
    int spl = splhigh();
    struct pid_clist *p;
    for (p = unavailable_pids; p != NULL; p = p->next) {
        if (p->pid == x) {
            splx(spl);
            return 1;
        }
    }
    splx(spl);
    return 0;
}

#endif
