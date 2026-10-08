#ifndef RENDER
#define RENDER 0
#endif
.section .text,"ax",@progbits
.globl main
.type main,@function
main:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    sw s1, 4(sp)
    sw s2, 0(sp)

    la a0, input_state
    la a1, solver_cube_state
    call parse_state_asm
    beqz a0, .Lsolver_invalid_input
#if RENDER
    la a0, solver_cube_state
    call render_cube_asm
#endif

    la a0, solver_cube_state
    call rank_coordinates_asm
    la a2, solver_path
    call ida_search_asm
    mv s0, a0

    la a0, solver_cube_state
    la a1, solver_path
    mv a2, s0
    call path_solves_asm
    beqz a0, .Lsolver_search_failed

    li s1, 0
    la s2, solver_path
.Lsolver_print_loop:
    bgeu s1, s0, .Lsolver_print_newline
    beqz s1, .Lsolver_print_move
    la a0, solver_space
    li a7, 4
    ecall
.Lsolver_print_move:
    add t0, s2, s1
    lbu t1, 0(t0)
    slli t1, t1, 2
    la t0, solver_move_names
    add t0, t0, t1
    lw a0, 0(t0)
    li a7, 4
    ecall
#if RENDER
    la a0, solver_cube_state
    add t0, s2, s1
    lbu a1, 0(t0)
    call apply_move_asm
    beqz a0, .Lsolver_search_failed

    la a0, solver_cube_state
    call render_cube_asm
#endif
    addi s1, s1, 1
    j .Lsolver_print_loop

.Lsolver_print_newline:
#if RENDER
    la a0, solver_cube_state
    la a1, solver_path
    li a2, 0
    call path_solves_asm
    beqz a0, .Lsolver_search_failed
#endif
    la a0, solver_newline
    li a7, 4
    ecall
    li a0, 0
    j .Lsolver_return
.Lsolver_invalid_input:
    la a0, solver_invalid_message
    li a7, 4
    ecall
    li a0, 2
    j .Lsolver_return
.Lsolver_search_failed:
    la a0, solver_search_message
    li a7, 4
    ecall
    li a0, 1
.Lsolver_return:
    lw s2, 0(sp)
    lw s1, 4(sp)
    lw s0, 8(sp)
    lw ra, 12(sp)
    addi sp, sp, 16
    ret
.size main, .-main

.section .rodata.main,"a",@progbits
.align 2
solver_move_names:
    .word solver_name_r
    .word solver_name_r2
    .word solver_name_r_inverse
    .word solver_name_b
    .word solver_name_b2
    .word solver_name_b_inverse
    .word solver_name_d
    .word solver_name_d2
    .word solver_name_d_inverse

solver_name_r:
    .asciz "R"
solver_name_r2:
    .asciz "R2"
solver_name_r_inverse:
    .asciz "R'"
solver_name_b:
    .asciz "B"
solver_name_b2:
    .asciz "B2"
solver_name_b_inverse:
    .asciz "B'"
solver_name_d:
    .asciz "D"
solver_name_d2:
    .asciz "D2"
solver_name_d_inverse:
    .asciz "D'"

solver_space:
    .asciz " "
solver_newline:
    .asciz "\n"
solver_invalid_message:
    .asciz "invalid input\n"
solver_search_message:
    .asciz "search or path validation failed\n"

.section .bss.main,"aw",@nobits
solver_cube_state:
    .space 14
solver_path:
    .space 11
