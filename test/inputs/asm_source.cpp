void source_gcc_asm(int input, int *output) {
  __asm__ volatile("movl %1, %0\n\t"
                   "addl $1, %0"
                   : "=r"(*output)
                   : "r"(input)
                   : "cc");
}

void source_ms_asm(int value) {
  __asm {
    mov eax, value
    jmp local_done
  local_done:
    add eax, 1
  }
}
