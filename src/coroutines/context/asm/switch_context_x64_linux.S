#if (APPLE)
  #define FUNCTION_NAME(name) _##name
#else
  #define FUNCTION_NAME(name) name
#endif

.global SwitchMachineContext

# SwitchMachineContext(from_rsp, to_rsp)

SwitchMachineContext:

    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12

    pushq %rbx
    pushq %rbp

    # Switch stacks

    # 1.2 Save current stack pointer to 'from' MachineContext
    movq %rsp, (%rdi)
    # from_rsp := rsp

    # 2. Activate 'to' machine context

    # 2.1 Set stack pointer to target stack
    movq (%rsi), %rsp
    # rsp := to_rsp

    # 2.2 Restore and pop registers saved on target stack

    popq %rbp
    popq %rbx

    popq %r12
    popq %r13
    popq %r14
    popq %r15

    # Pop current SwitchMachineContext frame from target stack

    retq
