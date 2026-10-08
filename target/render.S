#ifndef RENDER
#define RENDER 0
#endif

#if RENDER
.include "generated/led_symbols.inc"

.section .text.render,"ax",@progbits
.globl render_cube_asm
.type render_cube_asm,@function
# a0: valid 14-byte corner state. The state is preserved.
render_cube_asm:
    addi sp, sp, -16
    sw s0, 12(sp)
    sw s1, 8(sp)
    sw s2, 4(sp)
    sw s3, 0(sp)
    mv s0, a0
    li s2, LED_MATRIX_0_BASE
    li s3, LED_MATRIX_0_WIDTH
    slli s3, s3, 2

    # Clear the whole matrix, including separators and the bottom five rows.
    mv t0, s2
    li t1, LED_MATRIX_0_HEIGHT
.Lrender_clear_row:
    li t2, LED_MATRIX_0_WIDTH
.Lrender_clear_pixel:
    sw zero, 0(t0)
    addi t0, t0, 4
    addi t2, t2, -1
    bnez t2, .Lrender_clear_pixel
    addi t1, t1, -1
    bnez t1, .Lrender_clear_row

    li s1, 0
.Lrender_facelet:
    la t0, led_positions
    add t0, t0, s1
    lbu t0, 0(t0)
    li t1, 7
    beq t0, t1, .Lrender_fixed_corner
    add t0, s0, t0
    lbu t1, 0(t0)
    lbu t2, 7(t0)
    j .Lrender_lookup
.Lrender_fixed_corner:
    li t1, 7
    li t2, 0

.Lrender_lookup:
    # Offset = facelet * 24 + cubie * 3 + orientation.
    slli t0, s1, 4
    slli t3, s1, 3
    add t0, t0, t3
    slli t3, t1, 1
    add t3, t3, t1
    add t0, t0, t3
    add t0, t0, t2
    la t3, led_color_lut
    add t0, t0, t3
    lbu t4, 0(t0)

    slli t4, t4, 2
    la t0, led_palette
    add t0, t0, t4
    lw t4, 0(t0)

    # Find the facelet's pixel origin.
    slli t0, s1, 1
    la t3, led_origins
    add t0, t0, t3
    lbu t1, 0(t0)
    lbu t2, 1(t0)

    # Address = BASE + y * row_stride + x * 4.
    mv t3, s2
.Lrender_seek_row:
    beqz t2, .Lrender_origin_ready
    add t3, t3, s3
    addi t2, t2, -1
    j .Lrender_seek_row
.Lrender_origin_ready:
    slli t1, t1, 2
    add t3, t3, t1

    li t2, 3
.Lrender_tile_row:
    mv t0, t3
    li t1, 4
.Lrender_tile_pixel:
    sw t4, 0(t0)
    addi t0, t0, 4
    addi t1, t1, -1
    bnez t1, .Lrender_tile_pixel
    add t3, t3, s3
    addi t2, t2, -1
    bnez t2, .Lrender_tile_row

    addi s1, s1, 1
    li t0, 24
    bltu s1, t0, .Lrender_facelet

    lw s3, 0(sp)
    lw s2, 4(sp)
    lw s1, 8(sp)
    lw s0, 12(sp)
    addi sp, sp, 16
    ret
.size render_cube_asm, .-render_cube_asm

.include "generated/led_tables.inc"
#endif
