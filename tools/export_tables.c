/* Host-only tool: reuse the current table builders. */
#define main minirubik_host_main
#include "../solver.c"
#undef main

static void write_value(FILE *c, FILE *assembly, unsigned value,
                        unsigned index, unsigned count,
                        const char *directive)
{
    if (index % 16U == 0) {
        fputs("\n        ", c);
        fprintf(assembly, "    %s ", directive);
    }
    fprintf(c, "%u,", value);
    fprintf(assembly, "%u%s", value,
            index % 16U == 15U || index + 1U == count ? "\n" : ",");
}

static void write_matrix(FILE *c, FILE *assembly, const char *name,
                         uint16_t count, uint16_t table[3][count])
{
    fprintf(c, "static const uint16_t %s[3][%u] = {\n", name,
            (unsigned) count);
    fprintf(assembly, "\n.align 1\n%s:\n", name);
    unsigned maximum = 0;
    for (unsigned face = 0; face < 3; ++face) {
        fputs("    {", c);
        for (unsigned i = 0; i < count; ++i) {
            unsigned value = table[face][i];
            if (value > maximum)
                maximum = value;
            write_value(c, assembly, value, i, count, ".half");
        }
        fputs("\n    },\n", c);
    }
    fputs("};\n\n", c);
    printf("%s: entries=%u max=%u solved=%u,%u,%u\n", name,
           3U * count, maximum, (unsigned) table[0][0],
           (unsigned) table[1][0], (unsigned) table[2][0]);
}

static void write_distances(FILE *c, FILE *assembly, const char *name,
                            const uint8_t *table, unsigned count)
{
    fprintf(c, "static const uint8_t %s[%u] = {", name, count);
    fprintf(assembly, "\n%s:\n", name);
    unsigned maximum = 0;
    for (unsigned i = 0; i < count; ++i) {
        unsigned value = table[i];
        if (value > maximum)
            maximum = value;
        write_value(c, assembly, value, i, count, ".byte");
    }
    fputs("\n};\n\n", c);
    printf("%s: entries=%u max=%u solved=%u\n", name, count,
           maximum, (unsigned) table[0]);
}

int main(void)
{
    build_transitions();
    if (!build_heuristics()) {
        fputs("could not build small tables\n", stderr);
        return 1;
    }

    FILE *c = fopen("generated/tables.h", "w");
    FILE *assembly = fopen("generated/tables.inc", "w");
    if (!c || !assembly) {
        perror("open generated tables");
        if (c)
            fclose(c);
        if (assembly)
            fclose(assembly);
        return 1;
    }

    fputs("#ifndef MINIRUBIK_GENERATED_TABLES_H\n"
          "#define MINIRUBIK_GENERATED_TABLES_H\n"
          "#include <stdint.h>\n\n", c);
    fputs(".section .rodata,\"a\",@progbits\n", assembly);

    write_matrix(c, assembly, "permutation", PERMUTATIONS, permutation);
    write_matrix(c, assembly, "orientation", ORIENTATIONS, orientation);
    write_distances(c, assembly, "perm_distance", perm_distance, PERMUTATIONS);
    write_distances(c, assembly, "ori_distance", ori_distance, ORIENTATIONS);
    fputs("#endif\n", c);

    int failed = ferror(c) || ferror(assembly);
    if (fclose(c) != 0)
        failed = 1;
    if (fclose(assembly) != 0)
        failed = 1;
    if (failed) {
        fputs("could not write generated tables\n", stderr);
        return 1;
    }

    printf("table payload: %zu bytes\n",
           sizeof permutation + sizeof orientation +
           sizeof perm_distance + sizeof ori_distance);
    puts("generated/tables.h and generated/tables.inc written");
    return output_failed();
}
