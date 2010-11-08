#if OPT_A2

#include <pid.h>

#ifndef __CHILD_TABLE__
#define __CHILD_TABLE__
struct child_table {
    pid_t pid;
    volatile int finished; //failsafe for if someone uses -1 as an exit code
    int exit_code;
    struct child_table *next;
};
#endif
#endif
