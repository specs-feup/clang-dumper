#include "asm_inline_macro.h"
#define SOURCE_BASIC_ASM __asm__
#define SOURCE_INLINE_QUALIFIER __inline__
#define SOURCE_NESTED_INLINE_ASM SOURCE_BASIC_ASM SOURCE_INLINE_QUALIFIER volatile
#define SOURCE_PLAIN_ASM __asm__

void source_gcc_asm(int input, int *output) {
  __asm__ volatile("movl %1, %0\n\t"
                   "addl $1, %0"
                   : "=r"(*output)
                   : "r"(input)
                   : "cc");
}

void source_gcc_asm_goto(int input, int *output) {
  __asm__ goto("test %1, %1\n\t"
               "jz %l[zero]\n\t"
               "movl %1, %0"
               : "=r"(*output)
               : "r"(input)
               : "cc"
               : zero);
  return;
zero:
  return;
}

void source_gcc_asm_inline(void) {
  __asm__ inline volatile("nop");
}

void source_gcc_asm_inline_macro(void) {
  SOURCE_INLINE_ASM("nop");
}

void source_gcc_asm_inline_use_site(void) {
  SOURCE_BASIC_ASM inline volatile("nop");
}

void source_gcc_asm_inline_nested_macro(void) {
  SOURCE_NESTED_INLINE_ASM("nop");
}

inline void source_unrelated_inline(void) {}

void source_gcc_asm_noninline_macro(void) {
  SOURCE_PLAIN_ASM volatile("nop");
}

void source_gcc_asm_inline_spelling(void) {
  __asm__ __inline volatile("nop");
  __asm__ __inline__ volatile("nop");
}

void source_ms_asm(int value) {
  __asm {
    mov eax, value
    jmp local_done
  local_done:
    add eax, 1
  }
}
