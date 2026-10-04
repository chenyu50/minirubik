# a1: loop iteration count.
# The loop repeatedly accesses a fixed 4 KiB guest-memory region.

.text
.globl main
main:
    beqz a1, done

    li t0, 0x10000000
    li t1, 4095
    li t2, 0
    mv t3, a1
    li t4, 1

loop:
    add t5, t0, t2
    sw t4, 0(t5)
    lw t6, 0(t5)
    addi t4, t6, 1
    addi t2, t2, 4
    and t2, t2, t1
    addi t3, t3, -1
    bnez t3, loop

done:
    li a7, 10
    ecall