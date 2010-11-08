#include "opt-A2.h"
#if OPT_A2

#define MIN_PID 100 //the minimum pid **MUST BE GREATER THAN THE NUMBER OF ERROR CODES**

#include <types.h>

pid_t new_pid();
void pid_process_exit(pid_t x);
void pid_parent_done(pid_t x);
void pid_free(pid_t x);
int pid_claimed(pid_t x);

#endif
