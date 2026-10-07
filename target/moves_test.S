#ifndef PATH_LENGTH
#define PATH_LENGTH 11
#endif
#ifndef PATH_BYTES
#define PATH_BYTES 0,5,7,2,3,2,5,0,7,0,3
#endif
#ifndef EXPECT_SOLVES
#define EXPECT_SOLVES 1
#endif

.section .text,"ax",@progbits
.globl main
.type main,@function
main:
    addi sp, sp, -32
    sw ra, 28(sp)
    sw s0, 24(sp)
    sw s1, 20(sp)

    li s0, 0
.Lmoves_test_move:
    la a0, roundtrip_state
    la a1, solved_state
    call moves_test_copy14

    la a0, roundtrip_state
    mv a1, s0
    call apply_move_asm
    beqz a0, .Lmoves_test_fail

    la a0, roundtrip_state
    call rank_coordinates_asm
    la t0, expected_pairs
    slli t1, s0, 3
    add t0, t0, t1
    lw t1, 0(t0)
    bne a0, t1, .Lmoves_test_fail
    lw t1, 4(t0)
    bne a1, t1, .Lmoves_test_fail

    la t0, test_inverse_moves
    add t0, t0, s0
    lbu a1, 0(t0)
    la a0, roundtrip_state
    call apply_move_asm
    beqz a0, .Lmoves_test_fail

    la a0, roundtrip_state
    la a1, solved_state
    call moves_test_equal14
    beqz a0, .Lmoves_test_fail

    addi s0, s0, 1
    li t0, 9
    bltu s0, t0, .Lmoves_test_move

    la s1, cube_state
    la a0, input_state
    mv a1, s1
    call parse_state_asm
    beqz a0, .Lmoves_test_fail

    la a0, saved_state
    mv a1, s1
    call moves_test_copy14

    mv a0, s1
    la a1, test_path
    li a2, PATH_LENGTH
    call path_solves_asm
    li t0, EXPECT_SOLVES
    bne a0, t0, .Lmoves_test_fail

    mv a0, s1
    la a1, saved_state
    call moves_test_equal14
    beqz a0, .Lmoves_test_fail

    la a0, moves_test_pass_message
    li a7, 4
    ecall
    li a0, 0
    j .Lmoves_test_return
.Lmoves_test_fail:
    la a0, moves_test_fail_message
    li a7, 4
    ecall
    li a0, 1
.Lmoves_test_return:
    lw s1, 20(sp)
    lw s0, 24(sp)
    lw ra, 28(sp)
    addi sp, sp, 32
    ret
.size main, .-main

# a0: destination, a1: source.
.type moves_test_copy14,@function
moves_test_copy14:
    li t0, 14
.Lmoves_test_copy_loop:
    lbu t1, 0(a1)
    sb t1, 0(a0)
    addi a0, a0, 1
    addi a1, a1, 1
    addi t0, t0, -1
    bnez t0, .Lmoves_test_copy_loop
    ret
.size moves_test_copy14, .-moves_test_copy14

# a0/a1: state pointers. Return a0 = 1 if all bytes match.
.type moves_test_equal14,@function
moves_test_equal14:
    li t0, 14
.Lmoves_test_equal_loop:
    lbu t1, 0(a0)
    lbu t2, 0(a1)
    bne t1, t2, .Lmoves_test_unequal
    addi a0, a0, 1
    addi a1, a1, 1
    addi t0, t0, -1
    bnez t0, .Lmoves_test_equal_loop
    li a0, 1
    ret
.Lmoves_test_unequal:
    li a0, 0
    ret
.size moves_test_equal14, .-moves_test_equal14

.section .rodata.moves_test,"a",@progbits
.align 2
expected_pairs:
    .word 1104,426
    .word 3294,0
    .word 2190,426
    .word 9,16
    .word 16,0
    .word 18,16
    .word 198,0
    .word 566,0
    .word 368,0
solved_state:
    .byte 0,1,2,3,4,5,6,0,0,0,0,0,0,0
test_inverse_moves:
    .byte 2,1,0,5,4,3,8,7,6
test_path:
    .byte PATH_BYTES
moves_test_pass_message:
    .asciz "moves test passed\n"
moves_test_fail_message:
    .asciz "moves test failed\n"

.section .bss.moves_test,"aw",@nobits
roundtrip_state:
    .space 14
cube_state:
    .space 14
saved_state:
    .space 14
