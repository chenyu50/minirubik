/* Host-only: enumerate hard inputs using the original Benchmark. */
#define main minirubik_benchmark_main
#include "../benchmark.c"
#undef main

static int benchmark_distance(const uint8_t *table, state_t state)
{
    unsigned length = 0;
    uint32_t rank = rank_state(&state);

    while (rank != 0) {
        if (length >= 11 || table[rank] >= MOVES)
            return -1;
        state = apply_move(state, table[rank]);
        rank = rank_state(&state);
        ++length;
    }
    return (int) length;
}

int main(void)
{
    uint8_t diameter;
    uint8_t *table = build_table(&diameter);
    if (!table || diameter != 11) {
        fputs("could not build diameter-11 Benchmark\n", stderr);
        free(table);
        return 1;
    }

    FILE *out = fopen("tests/distance11.txt", "w");
    if (!out) {
        perror("tests/distance11.txt");
        free(table);
        return 1;
    }

    unsigned count = 0;
    int fixed_found = 0;
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        state_t state;
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank) {
            fputs("Benchmark state decoding failed\n", stderr);
            fclose(out);
            free(table);
            return 1;
        }

        int distance = benchmark_distance(table, state);
        if (distance < 0) {
            fputs("Benchmark path failed\n", stderr);
            fclose(out);
            free(table);
            return 1;
        }
        if (distance != 11)
            continue;

        char input[15];
        for (unsigned i = 0; i < CUBIES; ++i) {
            input[i] = (char) ('1' + state.p[i]);
            input[i + CUBIES] = (char) ('1' + state.o[i]);
        }
        input[14] = '\0';
        fprintf(out, "%s\n", input);
        ++count;
        if (!strcmp(input, "21345671111111"))
            fixed_found = 1;
    }

    int failed = ferror(out);
    if (fclose(out) != 0)
        failed = 1;
    free(table);
    if (failed || count != 2644 || !fixed_found) {
        fprintf(stderr, "export failed: count=%u fixed=%d\n",
                count, fixed_found);
        return 1;
    }

    printf("Benchmark: states=%u diameter=%u\n",
           (unsigned) STATES, (unsigned) diameter);
    printf("distance-11 inputs exported: %u\n", count);
    puts("fixed input included: yes");
    puts("output: tests/distance11.txt");
    return fflush(stdout) != 0 || ferror(stdout);
}
