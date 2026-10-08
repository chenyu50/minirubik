#if !RENDER
#error "Build the GUI renderer test with -DRENDER=1"
#endif

.section .text,"ax",@progbits
.globl main
.type main,@function
main:
    addi sp, sp, -16
    sw ra, 12(sp)

    la a0, input_state
    la a1, render_test_state
    call parse_state_asm
    beqz a0, .Lrender_test_invalid

    la a0, render_test_state
    call render_cube_asm
    la a0, render_test_message
    li a7, 4
    ecall
    li a0, 0
    j .Lrender_test_return

.Lrender_test_invalid:
    la a0, render_test_invalid_message
    li a7, 4
    ecall
    li a0, 2

.Lrender_test_return:
    lw ra, 12(sp)
    addi sp, sp, 16
    ret
.size main, .-main

.section .rodata.render_test,"a",@progbits
render_test_message:
    .asciz "LED net stores completed\n"
render_test_invalid_message:
    .asciz "invalid input\n"

.section .bss.render_test,"aw",@nobits
render_test_state:
    .space 14
