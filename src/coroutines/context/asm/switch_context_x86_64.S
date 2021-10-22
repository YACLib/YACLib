#include "switch_context_x86_64.hpp"

.text
.globl YACLIB_FUNC_NAME(__yaclib_switch_context)
YACLIB_FUNC_NAME(__yaclib_switch_context):
    // rdi = from
    // rsi = to

    // store from registers
    movq        %rbx, (YACLIB_RBX_INDEX * 8)(%rdi)
    movq        %rbp, (YACLIB_RBP_INDEX * 8)(%rdi)
    movq        %r12, (YACLIB_R12_INDEX * 8)(%rdi)
    movq        %r13, (YACLIB_R13_INDEX * 8)(%rdi)
    movq        %r14, (YACLIB_R14_INDEX * 8)(%rdi)
    movq        %r15, (YACLIB_R15_INDEX * 8)(%rdi)

    movq        (%rsp), %r8  // get return agress
    movq        %r8, (YACLIB_RIP_INDEX * 8)(%rdi)
    leaq        8(%rsp), %r8 // get rsp
    movq        %r8, (YACLIB_RSP_INDEX * 8)(%rdi)

    // load to registers
    movq        (YACLIB_RBX_INDEX * 8)(%rsi), %rbx
    movq        (YACLIB_RBP_INDEX * 8)(%rsi), %rbp
    movq        (YACLIB_R12_INDEX * 8)(%rsi), %r12
    movq        (YACLIB_R13_INDEX * 8)(%rsi), %r13
    movq        (YACLIB_R14_INDEX * 8)(%rsi), %r14
    movq        (YACLIB_R15_INDEX * 8)(%rsi), %r15

    movq        (YACLIB_RSP_INDEX * 8)(%rsi), %rsp
    movq        (YACLIB_RIP_INDEX * 8)(%rsi), %rcx
    // for trampoline
    movq        (YACLIB_RDI_INDEX * 8)(%rsi), %rdi
    movq        (YACLIB_RSI_INDEX * 8)(%rsi), %rsi
    jmp         *%rcx
