#ifndef PSXTHREAD_H_
#define PSXTHREAD_H_

/**
 * Searches a free TCB, marks it as used, and stores the initial
 * program counter (PC), global pointer (GP aka R28), stack pointer
 * (SP aka R29), and frame pointer (FP aka R30) (using the same value
 * for SP and FP). All other registers are left uninitialized
 * (eg. may contain values from an older closed thread, that includes
 * the SR register, see note).
 *
 * \return  New thread handle (in range FF000000h..FF000003h, assuming
 * that 4 TCBs are allocated) or FFFFFFFFh if there's no free TCB.
 *
 * \remarks The function returns to the old current thread, use
 * "ChangeThread" to switch to the new thread.
 *
 * \note    The desired max number of TCBs can be specified in the
 * SYSTEM.CNF boot file (the default is "TCB = 4", one initially used
 * for the boot executable, plus 3 free threads).
 */
unsigned int OpenThread(unsigned int reg_PC, unsigned int reg_SP_FP, unsigned int reg_GP);

/**
 * Pauses the current thread, and activates the selected new thread
 * (or crashes if the specified handle was unused or invalid).
 *
 * \return The return value is always 1 (stored in the R2 entry of the TCB
 * of the old thread, so the return value will be received once when
 * changing back to the old thread).
 *
 * \note    The BIOS doesn't automatically switch from one thread to another.
 * So, all other threads remain paused until the current thread uses
 * ChangeThread to pass control to another thread.
 * Each thread is having it's own CPU registers (R1..R31,HI,LO,SR,PC),
 * the registers are stored in the TCB of the old thread, and restored when
 * switching back to that thread. Mind that other registers (I/O Ports or GTE
 * registers aren't stored automatically, so, when needed, they need to be
 * pushed/popped by software before/after ChangeThread).
 */
unsigned int ChangeThread(unsigned int handle);

/**
 * Returns global pointer value aka $gp or $r28.
 *
 * \return  Global pointer.
 */
unsigned int GetGP(void);

#endif /* PSXTHREAD_H_ */
