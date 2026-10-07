#ifndef INPUT_STATE
#define INPUT_STATE "21345671111111"
#endif

.section .text.start,"ax",@progbits
.globl _start
.type _start,@function
_start:
    .option push
    .option norelax
    la gp, __global_pointer$
    la sp, __stack_top
    .option pop

    la t0, __bss_start
    la t1, __bss_end
1:
    bgeu t0, t1, 2f
    sw zero, 0(t0)
    addi t0, t0, 4
    j 1b
2:
    call main
    li a7, 93
    ecall
3:
    j 3b
.size _start, .-_start

.section .rodata.input,"a",@progbits
.globl input_state
input_state:
    .asciz INPUT_STATE
