# a1: number of guest bytes to write.
# Input must be positive and divisible by 4.

.text
.globl main
main:
    li t0, 0x10000000
    add t1, t0, a1
    li t2, 0x01010101

fill:
    sw t2, 0(t0)
    addi t0, t0, 4
    bltu t0, t1, fill

    li a7, 10
    ecall