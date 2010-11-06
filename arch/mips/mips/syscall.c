#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <machine/specialreg.h>
#include <kern/callno.h>
#include <syscall.h>
#include "opt-A2.h"

#if OPT_A2
#include <pid.h>
#include <thread.h>
#include <curthread.h>
#endif

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0 register, like an ordinary function call, and the a3 register is also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0 register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and returning the value -1 from the actual userlevel syscall function. See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe must be incremented by one instruction; otherwise the exception return code will restart the "syscall" instruction and the system call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments, there should be no need to fetch additional arguments from the user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as arguments, they will get passed in pairs of registers, and not necessarily in the way you expect. We recommend you don't do it. 
 * (In fact, we recommend you don't use 64-bit quantities at all. See arch/mips/include/types.h.)
 */

void mips_syscall(struct trapframe *tf) {
    int callno;
    int32_t retval;
    int err;

    assert(curspl == 0);

    callno = tf->tf_v0;

    /*
     * Initialize retval to 0. Many of the system calls don't really return a value,
     * just 0 for success and -1 on error. Since retval is the value returned on success,
     * initialize it to 0 by default; thus it's not necessary to deal with it except
     * for calls that return other values, like write.
     */

    retval = 0;
    
    ///DEBUG(DB_A2FC, "DEBUG: Thread `%s` entering system call.\n", curthread->t_name);
    //commented out the above message to avoid printing a billion messages and taking forever and a half to execute

    switch (callno) {

#if OPT_A2
        case SYS__exit:
            //0
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling _exit\n", curthread->t_name);
            sys__exit(tf->tf_a0);
            break;

        case SYS_execv:
            //1
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling execv.\n", curthread->t_name);
            err = sys_execv((char*) tf->tf_a0, (char **) tf->tf_a1);
            break;

        case SYS_fork:
            //2
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling fork.\n", curthread->t_name);
            err = sys_fork(tf);
            DEBUG(DB_A2FC, "DEBUG: Fork code completed.\n");
            //the below is necessary since fork actually returns a value
            if (err >= MIN_PID) {
                retval = err;
                err = 0;
            };
            break;

        case SYS_waitpid:
            //3
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling waitpid.\n", curthread->t_name);
            err = sys_waitpid((pid_t) tf->tf_a0, &retval, tf->tf_a1);
            break;

        case SYS_open:
            //4
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling open.\n", curthread->t_name);
            err = sys_open(&retval, (char *) tf->tf_a0, (int) tf->tf_a1, (int) tf->tf_a2);
            break;

        case SYS_read:
            //5
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling read.\n", curthread->t_name);
            err = sys_read(&retval, tf->tf_a0, (void*) tf->tf_a1, tf->tf_a2);
            break;

        case SYS_write:
            //6
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling write.\n", curthread->t_name);
            err = sys_write(&retval, tf->tf_a0, (void*) tf->tf_a1, tf->tf_a2);

            break;

        case SYS_close:
            //7
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling close.\n", curthread->t_name);
            err = sys_close(&retval, tf->tf_a0);

            break;

#endif /* OPT_A2 */
        case SYS_reboot:
            //8
            #if OPT_A2
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling reboot.\n", curthread->t_name);
            #endif
            err = sys_reboot(tf->tf_a0);
            break;

#if OPT_A2
        case SYS_getpid:
            //11
            ///DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling getpid.\n", curthread->t_name);
            //commented out about debug to avoid huge number of messages as above
            err = sys_getpid();
            break;

#endif /* OPT_A2 */
        default:
            #if OPT_A2
            DEBUG(DB_A2FC, "DEBUG: Thread `%s` calling invalid system call.\n", curthread->t_name);
            #endif
            kprintf("Unknown syscall %d\n", callno);
            err = ENOSYS;
            break;
    }

    if (err) {
        /*
         * Return the error code. This gets converted at userlevel to a return value of -1 and the error code in errno.
         */
        tf->tf_v0 = err;
        tf->tf_a3 = 1; /* signal an error */
    } else {
        /* Success. */
        tf->tf_v0 = retval;
        tf->tf_a3 = 0; /* signal no error */
    }

    /*
     * Now, advance the program counter, to avoid restarting the syscall over and over again.
     */

    tf->tf_epc += 4;

    /* Make sure the syscall code didn't forget to lower spl */
    assert(curspl == 0);
}

void md_forkentry(struct trapframe *tf) {
    #if OPT_A2
    DEBUG(DB_A2FC, "DEBUG: Thread `%s` entering md_forkentry.\n", curthread->t_name);
    assert(curspl == 0);
    struct trapframe my_trap = *tf;
    my_trap.tf_v0 = 0; //set return value to 0
    my_trap.tf_status = CST_IRQMASK | CST_IEp | CST_KUp; //not sure if this is necessary
    ///tf_vaddr might need to be changed too, but I don't think so
    ///tf_sp might also need to be changed
    my_trap.tf_epc += 4; //increment program counter
    mips_usermode(&my_trap);
    assert(0); //should not get here. mips_usermode does not return
    #else
    (void *) tf;
    #endif
}
