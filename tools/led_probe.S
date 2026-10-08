.include "generated/led_symbols.inc"

.section .text.start,"ax",@progbits
.globl _start
_start:
    # Clear every LED to black.
    li t0, LED_MATRIX_0_BASE
    li t1, LED_MATRIX_0_HEIGHT
.Lled_clear_row:
    li t2, LED_MATRIX_0_WIDTH
.Lled_clear_pixel:
    sw zero, 0(t0)
    addi t0, t0, 4
    addi t2, t2, -1
    bnez t2, .Lled_clear_pixel
    addi t1, t1, -1
    bnez t1, .Lled_clear_row

    # Red at (0, 0), green at (1, 0).
    li t0, LED_MATRIX_0_BASE
    li t1, 0x00ff0000
    sw t1, 0(t0)
    li t1, 0x0000ff00
    sw t1, 4(t0)

    # Blue at (0, 1): one complete row after the base.
    li t2, LED_MATRIX_0_WIDTH
    slli t2, t2, 2
    add t2, t0, t2
    li t1, 0x000000ff
    sw t1, 0(t2)

    la a0, led_probe_message
    li a7, 4
    ecall
    li a0, 0
    li a7, 93
    ecall
.Lled_probe_halt:
    j .Lled_probe_halt

.section .rodata,"a",@progbits
led_probe_message:
    .asciz "LED probe stores completed\n"
