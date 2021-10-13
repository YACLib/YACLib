.text
.globl _SwitchMachineContext
_SwitchMachineContext:
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12

    pushq %rbx
    pushq %rbp

    /* Save current stack pointer to 'from' MachineContext */
    movq %rsp, (%rdi)

    /* Set stack pointer to target stack */
    movq (%rsi), %rsp

    popq %rbp
    popq %rbx

    popq %r12
    popq %r13
    popq %r14
    popq %r15

    retq