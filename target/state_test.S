#ifndef EXPECT_VALID
#define EXPECT_VALID 1
#endif
#ifndef EXPECTED_P
#define EXPECTED_P 720
#endif
#ifndef EXPECTED_O
#define EXPECTED_O 0
#endif

.section .text,"ax",@progbits
.globl main
.type main,@function
main:
    addi sp, sp, -16
    sw ra, 12(sp)

    la a0, input_state
    la a1, cube_state
    call parse_state_asm

    la t0, expected
    lw t1, 0(t0)
    bne a0, t1, .Ltest_failed
    beqz a0, .Ltest_passed

    la a0, cube_state
    call rank_coordinates_asm

    la t0, expected
    lw t1, 4(t0)
    bne a0, t1, .Ltest_failed
    lw t1, 8(t0)
    bne a1, t1, .Ltest_failed

.Ltest_passed:
    la a0, pass_message
    li a7, 4
    ecall
    li a0, 0
    j .Ltest_return

.Ltest_failed:
    la a0, fail_message
    li a7, 4
    ecall
    li a0, 1

.Ltest_return:
    lw ra, 12(sp)
    addi sp, sp, 16
    ret
.size main, .-main

.section .rodata,"a",@progbits
.align 2
expected:
    .word EXPECT_VALID, EXPECTED_P, EXPECTED_O
pass_message:
    .asciz "ASM state test passed\n"
fail_message:
    .asciz "ASM state test failed\n"

.section .bss,"aw",@nobits
cube_state:
    .space 14
