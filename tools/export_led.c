#define main minirubik_benchmark_main
#include "../benchmark.c"
#undef main
#include "../generated/ripes_system.h"

enum { U, L, F, R, B, D };

/* Each row lists the three faces in the corner's orientation order.
 * Corner 7 is the fixed FUL corner; state_t stores only corners 0..6.
 */
static const uint8_t corner_faces[8][3] = {
    {U, F, R}, {D, R, F}, {D, F, L}, {U, R, B},
    {D, B, R}, {D, L, B}, {U, B, L}, {U, L, F}
};

/* x: right, y: up, z: front. */
static const int corner_xyz[8][3] = {
    { 1,  1,  1}, { 1, -1,  1}, {-1, -1,  1}, { 1,  1, -1},
    { 1, -1, -1}, {-1, -1, -1}, {-1,  1, -1}, {-1,  1,  1}
};
static const int face_normals[6][3] = {
    {0, 1, 0}, {-1, 0, 0}, {0, 0, 1},
    {1, 0, 0}, {0, 0, -1}, {0, -1, 0}
};

/* Facelets within each face: top-left, top-right, bottom-left, bottom-right. */
static const uint8_t face_positions[6][4] = {
    {6, 3, 7, 0}, {6, 7, 5, 2}, {7, 0, 2, 1},
    {0, 3, 1, 4}, {3, 6, 4, 5}, {2, 1, 5, 4}
};
static const uint8_t face_origins[6][2] = {
    {9, 0}, {0, 7}, {9, 7}, {18, 7}, {27, 7}, {9, 14}
};
static const uint32_t palette[6] = {
    0x00ffffff, 0x00ff8000, 0x0000ff00,
    0x00ff0000, 0x000000ff, 0x00ffff00
};

static void rotate_vector(unsigned face, const int before[3], int after[3])
{
    if (face == 0) {              /* R: -90 degrees around x. */
        after[0] = before[0];
        after[1] = before[2];
        after[2] = -before[1];
    } else if (face == 1) {       /* B: +90 degrees around z. */
        after[0] = -before[1];
        after[1] = before[0];
        after[2] = before[2];
    } else {                     /* D: +90 degrees around y. */
        after[0] = before[2];
        after[1] = before[1];
        after[2] = -before[0];
    }
}

static int same_vector(const int a[3], const int b[3])
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

/* Compare the Benchmark's corner mapping with physical 3D rotations. */
static int check_geometry(unsigned *positions, unsigned *stickers)
{
    *positions = 0;
    *stickers = 0;
    for (unsigned face = 0; face < 3; ++face) {
        for (unsigned to = 0; to < CUBIES; ++to) {
            unsigned from = source[face][to];
            int active = face == 0 ? corner_xyz[from][0] == 1 :
                         face == 1 ? corner_xyz[from][2] == -1 :
                                     corner_xyz[from][1] == -1;
            int moved[3];
            if (active)
                rotate_vector(face, corner_xyz[from], moved);
            else
                memcpy(moved, corner_xyz[from], sizeof moved);
            if (!same_vector(moved, corner_xyz[to]))
                return 0;
            ++*positions;

            for (unsigned o = 0; o < 3; ++o) {
                for (unsigned k = 0; k < 3; ++k) {
                    unsigned before = corner_faces[from][(k + o) % 3];
                    unsigned after =
                        corner_faces[to][(k + o + twist[face][to]) % 3];
                    if (active)
                        rotate_vector(face, face_normals[before], moved);
                    else
                        memcpy(moved, face_normals[before], sizeof moved);
                    if (!same_vector(moved, face_normals[after]))
                        return 0;
                    ++*stickers;
                }
            }
        }
    }
    return 1;
}

static void write_bytes(FILE *out, const char *name,
                        const uint8_t *bytes, unsigned count)
{
    fprintf(out, "%s:\n", name);
    for (unsigned i = 0; i < count; ++i) {
        if (i % 12 == 0)
            fputs("    .byte ", out);
        fprintf(out, "%u%s", (unsigned) bytes[i],
                i % 12 == 11 || i + 1 == count ? "\n" : ",");
    }
}

int main(void)
{
    uint8_t positions[24], origins[48], colors[24 * 8 * 3];
    unsigned position_checks, sticker_checks, solved_checks = 0, maximum = 0;

    if (LED_MATRIX_0_WIDTH != 35 || LED_MATRIX_0_HEIGHT != 25) {
        fputs("LED Matrix must be 35 wide and 25 high\n", stderr);
        return 1;
    }
    if (!check_geometry(&position_checks, &sticker_checks)) {
        fputs("corner geometry does not match the Benchmark\n", stderr);
        return 1;
    }

    for (unsigned face = 0; face < 6; ++face) {
        for (unsigned cell = 0; cell < 4; ++cell) {
            unsigned n = face * 4 + cell;
            unsigned pos = face_positions[face][cell];
            unsigned j = 0;
            while (j < 3 && corner_faces[pos][j] != face)
                ++j;
            if (j == 3) {
                fputs("facelet refers to a corner outside its face\n", stderr);
                return 1;
            }
            positions[n] = (uint8_t) pos;
            origins[n * 2] =
                (uint8_t) (face_origins[face][0] + (cell % 2) * 4);
            origins[n * 2 + 1] =
                (uint8_t) (face_origins[face][1] + (cell / 2) * 3);
            if (origins[n * 2] + 4 > LED_MATRIX_0_WIDTH ||
                origins[n * 2 + 1] + 3 > LED_MATRIX_0_HEIGHT) {
                fputs("facelet exceeds LED Matrix bounds\n", stderr);
                return 1;
            }

            for (unsigned cubie = 0; cubie < 8; ++cubie) {
                for (unsigned o = 0; o < 3; ++o) {
                    unsigned color = corner_faces[cubie][(j + 3 - o) % 3];
                    colors[n * 24 + cubie * 3 + o] = (uint8_t) color;
                    if (color > maximum)
                        maximum = color;
                }
            }
            if (colors[n * 24 + pos * 3] != face) {
                fputs("solved facelet has the wrong color\n", stderr);
                return 1;
            }
            ++solved_checks;
        }
    }

    FILE *out = fopen("generated/led_tables.inc", "w");
    if (!out) {
        perror("generated/led_tables.inc");
        return 1;
    }
    fputs("# Generated by tools/export_led.c; do not edit manually.\n"
          ".section .rodata.led,\"a\",@progbits\n", out);
    write_bytes(out, "led_positions", positions, sizeof positions);
    write_bytes(out, "led_origins", origins, sizeof origins);
    write_bytes(out, "led_color_lut", colors, sizeof colors);
    fputs(".balign 4\nled_palette:\n", out);
    for (unsigned i = 0; i < 6; ++i)
        fprintf(out, "    .word 0x%08x\n", (unsigned) palette[i]);
    int failed = ferror(out);
    if (fclose(out) != 0)
        failed = 1;
    if (failed) {
        fputs("failed to write LED tables\n", stderr);
        return 1;
    }

    printf("LED geometry: %u corner-position and %u sticker rotations passed\n",
           position_checks, sticker_checks);
    printf("LED color lookup: entries=%u max=%u solved facelets=%u\n",
           (unsigned) sizeof colors, maximum, solved_checks);
    printf("LED table payload: %u bytes\n",
           (unsigned) (sizeof positions + sizeof origins +
                       sizeof colors + sizeof palette));
    puts("generated/led_tables.inc written");
    return fflush(stdout) != 0 || ferror(stdout);
}
