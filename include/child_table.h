#if OPT_A2

#include <pid.h>

#ifndef __CHILD_TABLE__
#define __CHILD_TABLE__
struct child_table {
    pid_t pid;
    int finished;
    struct child_table *next;
}
#endif
#endif
