.section .text,"ax",@progbits
.globl quarter_turn_asm
.type quarter_turn_asm,@function
# a0: valid state, a1: face 0..2. Update the state in place.
quarter_turn_asm:
    addi sp, sp, -16
    mv t0, a0
    mv t1, sp
    li t2, 14
.Lquarter_copy:
    lbu t3, 0(t0)
    sb t3, 0(t1)
    addi t0, t0, 1
    addi t1, t1, 1
    addi t2, t2, -1
    bnez t2, .Lquarter_copy

    slli t0, a1, 3
    sub t0, t0, a1
    la t1, move_source
    add t1, t1, t0
    la t2, move_twist
    add t2, t2, t0
    mv t3, a0
    li t4, 7
.Lquarter_update:
    lbu t5, 0(t1)
    add t6, sp, t5
    lbu t5, 0(t6)
    sb t5, 0(t3)
    lbu t5, 7(t6)
    lbu t6, 0(t2)
    add t5, t5, t6
    li t6, 3
    bltu t5, t6, .Lquarter_store
    addi t5, t5, -3
.Lquarter_store:
    sb t5, 7(t3)
    addi t1, t1, 1
    addi t2, t2, 1
    addi t3, t3, 1
    addi t4, t4, -1
    bnez t4, .Lquarter_update
    addi sp, sp, 16
    ret
.size quarter_turn_asm, .-quarter_turn_asm

.globl apply_move_asm
.type apply_move_asm,@function
# a0: valid state, a1: move ID.
# Return a0 = 1 after updating, or 0 for an invalid move ID.
apply_move_asm:
    sltiu t0, a1, 9
    beqz t0, .Lmove_invalid

    addi sp, sp, -32
    sw ra, 28(sp)
    sw s0, 24(sp)
    sw s1, 20(sp)
    sw s2, 16(sp)
    mv s0, a0
    li s1, 0
    li t0, 6
    bltu a1, t0, .Lmove_check_b
    li s1, 2
    addi a1, a1, -6
    j .Lmove_decoded
.Lmove_check_b:
    li t0, 3
    bltu a1, t0, .Lmove_decoded
    li s1, 1
    addi a1, a1, -3
.Lmove_decoded:
    addi s2, a1, 1
.Lmove_turn:
    mv a0, s0
    mv a1, s1
    call quarter_turn_asm
    addi s2, s2, -1
    bnez s2, .Lmove_turn

    li a0, 1
    lw s2, 16(sp)
    lw s1, 20(sp)
    lw s0, 24(sp)
    lw ra, 28(sp)
    addi sp, sp, 32
    ret
.Lmove_invalid:
    li a0, 0
    ret
.size apply_move_asm, .-apply_move_asm

.globl path_solves_asm
.type path_solves_asm,@function
# a0: initial state, a1: path, a2: length.
# Return a0 = 1 if solved, otherwise 0. Preserve the initial state.
path_solves_asm:
    li t0, 11
    bltu t0, a2, .Lpath_invalid
    addi sp, sp, -32
    sw ra, 28(sp)
    sw s0, 24(sp)
    sw s1, 20(sp)
    mv s0, a1
    mv s1, a2

    mv t0, a0
    mv t1, sp
    li t2, 14
.Lpath_copy:
    lbu t3, 0(t0)
    sb t3, 0(t1)
    addi t0, t0, 1
    addi t1, t1, 1
    addi t2, t2, -1
    bnez t2, .Lpath_copy

.Lpath_move:
    beqz s1, .Lpath_check
    mv a0, sp
    lbu a1, 0(s0)
    call apply_move_asm
    beqz a0, .Lpath_failed
    addi s0, s0, 1
    addi s1, s1, -1
    j .Lpath_move

.Lpath_check:
    mv t0, sp
    li t1, 0
    li t3, 7
.Lpath_check_corner:
    lbu t2, 0(t0)
    bne t2, t1, .Lpath_failed
    lbu t2, 7(t0)
    bnez t2, .Lpath_failed
    addi t0, t0, 1
    addi t1, t1, 1
    bltu t1, t3, .Lpath_check_corner
    li a0, 1
    j .Lpath_return
.Lpath_failed:
    li a0, 0
.Lpath_return:
    lw s1, 20(sp)
    lw s0, 24(sp)
    lw ra, 28(sp)
    addi sp, sp, 32
    ret
.Lpath_invalid:
    li a0, 0
    ret
.size path_solves_asm, .-path_solves_asm

.section .rodata,"a",@progbits
move_source:
    .byte 1,4,2,0,3,5,6
    .byte 0,1,2,4,5,6,3
    .byte 0,2,5,3,1,4,6
move_twist:
    .byte 1,2,0,2,1,0,0
    .byte 0,0,0,1,2,1,2
    .byte 0,0,0,0,0,0,0
