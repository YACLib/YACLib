#include "switch_context_x86_64.hpp"

// clang-format off
.text
.globl YACLIB_FUNC_NAME(yaclib_switch_context)
YACLIB_FUNC_NAME(yaclib_switch_context):
  // rdi = from
  // rsi = to

  // store from registers
  movq    %rbx, (YACLIB_RBX_INDEX * 8)(%rdi)
  movq    %rbp, (YACLIB_RBP_INDEX * 8)(%rdi)
  movq    %r12, (YACLIB_R12_INDEX * 8)(%rdi)
  movq    %r13, (YACLIB_R13_INDEX * 8)(%rdi)
  movq    %r14, (YACLIB_R14_INDEX * 8)(%rdi)
  movq    %r15, (YACLIB_R15_INDEX * 8)(%rdi)

  movq  (%rsp), %rax  // get return agress
  movq    %rax, (YACLIB_RIP_INDEX * 8)(%rdi)
  leaq 8(%rsp), %rax // get rsp
  movq    %rax, (YACLIB_RSP_INDEX * 8)(%rdi)

  // load to registers
  movq (YACLIB_RBX_INDEX * 8)(%rsi), %rbx
  movq (YACLIB_RBP_INDEX * 8)(%rsi), %rbp
  movq (YACLIB_R12_INDEX * 8)(%rsi), %r12
  movq (YACLIB_R13_INDEX * 8)(%rsi), %r13
  movq (YACLIB_R14_INDEX * 8)(%rsi), %r14
  movq (YACLIB_R15_INDEX * 8)(%rsi), %r15

  movq (YACLIB_RSP_INDEX * 8)(%rsi), %rsp
  movq (YACLIB_RIP_INDEX * 8)(%rsi), %rax

  jmp  *%rax


.globl YACLIB_FUNC_NAME(yaclib_trampoline)
YACLIB_FUNC_NAME(yaclib_trampoline):
  pushq %rbp
  movq  %rsp, %rbp
  movq  16(%rbp), %rax
  movq  8(%rbp), %rdi
  call  *%rax
