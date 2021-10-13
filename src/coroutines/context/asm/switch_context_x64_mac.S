.text
.globl _SwitchMachineContext
.align 8
_SwitchMachineContext:
    leaq  -0x38(%rsp), %rsp /* prepare stack */

    movq  %r12, 0x8(%rsp)  /* save R12 */
    movq  %r13, 0x10(%rsp)  /* save R13 */
    movq  %r14, 0x18(%rsp)  /* save R14 */
    movq  %r15, 0x20(%rsp)  /* save R15 */
    movq  %rbx, 0x28(%rsp)  /* save RBX */
    movq  %rbp, 0x30(%rsp)  /* save RBP */

    /* Save current stack pointer to 'from' MachineContext */
    movq  %rsp, %rdi

    /* Set stack pointer to target stack */
    movq  %rsi, %rsp

    movq  0x8(%rsp), %r12  /* restore R12 */
    movq  0x10(%rsp), %r13  /* restore R13 */
    movq  0x18(%rsp), %r14  /* restore R14 */
    movq  0x20(%rsp), %r15  /* restore R15 */
    movq  0x28(%rsp), %rbx  /* restore RBX */
    movq  0x30(%rsp), %rbp  /* restore RBP */

    /* leaq  0x40(%rsp), %rsp /* prepare stack */

    retq