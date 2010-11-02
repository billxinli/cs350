#include "opt-A2.h"
#if OPT_A2

#include <types.h>

pid_t new_pid();
void pid_process_exit(pid_t x);
void pid_parent_done(pid_t x);
void pid_free(pid_t x);

#endif
