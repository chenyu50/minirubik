#include <stdint.h>
#ifndef MINIRUBIK_TARGET
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    STATES = PERMUTATIONS * ORIENTATIONS,
    MOVES = 9
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;

/*@ predicate valid_state(state_t *state) =
      (\forall integer i; 0 <= i < CUBIES ==>
         state->p[i] < CUBIES && state->o[i] < 3) &&
      (\forall integer i, j; 0 <= i < j < CUBIES ==>
         state->p[i] != state->p[j]) &&
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
 */

static const char *const move_names[MOVES] = {"R",  "R2", "R'", "B", "B2",
                                              "B'", "D",  "D2", "D'"};
#ifndef MINIRUBIK_TARGET
static const uint8_t inverse_move[MOVES] = {2, 1, 0, 5, 4, 3, 8, 7, 6};
#endif
/* Each destination takes a cubie from source[face][destination]. */
static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t twist[3][CUBIES] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};

#ifdef MINIRUBIK_TARGET
#include "generated/tables.h"
#else
static uint16_t permutation[3][PERMUTATIONS];
static uint16_t orientation[3][ORIENTATIONS];

static uint8_t perm_distance[PERMUTATIONS];
static uint8_t ori_distance[ORIENTATIONS];
#endif

/* The three quarter-turns preserve the fixed front-upper-left corner. */
/*@ requires face < 3;
    requires \forall integer i; 0 <= i < CUBIES ==> state.o[i] < 3;
    assigns \nothing;
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.p[i] == state.p[source[face][i]];
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.o[i] == (state.o[source[face][i]] + twist[face][i]) % 3;
 */
static state_t quarter_turn(state_t state, uint8_t face)
{
    state_t result;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant \forall integer j; 0 <= j < i ==>
          result.p[j] == state.p[source[face][j]];
        loop invariant \forall integer j; 0 <= j < i ==>
          result.o[j] == (state.o[source[face][j]] + twist[face][j]) % 3;
        loop assigns i, result.p[0..6], result.o[0..6];
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t from = source[face][i];
        result.p[i] = state.p[from];
        uint8_t value = (uint8_t) (state.o[from] + twist[face][i]);
        if (value >= 3)
            value = (uint8_t) (value - 3U);
        result.o[i] = value;
    }
    return result;
}

static state_t apply_move(state_t state, uint8_t move)
{
    uint8_t face = 0;
    if (move >= 6) {
        face = 2;
        move = (uint8_t) (move - 6U);
    } else if (move >= 3) {
        face = 1;
        move = (uint8_t) (move - 3U);
    }
    uint8_t turns = (uint8_t) (move + 1U);
    for (uint8_t i = 0; i < turns; ++i)
        state = quarter_turn(state, face);
    return state;
}

#ifndef MINIRUBIK_TARGET
/*@ requires \valid_read(state);
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->p[i] < CUBIES;
    requires \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->o[i] < 3;
    assigns \nothing;
    ensures \result < STATES;
 */
static uint32_t rank_state(const state_t *state)
{
    uint32_t p = 0, o = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant (i == 0 ==> p == 0) && (i == 1 ==> p <= 6) &&
          (i == 2 ==> p <= 41) && (i == 3 ==> p <= 209) &&
          (i == 4 ==> p <= 839) && (i == 5 ==> p <= 2519) &&
          (i >= 6 ==> p <= 5039);
        loop assigns i, p;
        loop variant CUBIES - i;
     */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t smaller = 0;
        /*@ loop invariant i + 1 <= j <= CUBIES;
            loop invariant smaller <= j - i - 1;
            loop assigns j, smaller;
            loop variant CUBIES - j;
         */
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j)
            if (state->p[j] < state->p[i])
                ++smaller;
        p = p * (CUBIES - i) + smaller;
    }
    /*@ loop invariant 0 <= i <= 6;
        loop invariant (i == 0 ==> o == 0) && (i == 1 ==> o < 3) &&
          (i == 2 ==> o < 9) && (i == 3 ==> o < 27) &&
          (i == 4 ==> o < 81) && (i == 5 ==> o < 243) &&
          (i == 6 ==> o < 729);
        loop assigns i, o;
        loop variant 6 - i;
     */
    for (uint8_t i = 0; i < 6; ++i)
        o = o * 3U + state->o[i];
    return p * ORIENTATIONS + o;
}
#endif

static void rank_coordinates(const state_t *state,
                             uint16_t *permutation_rank,
                             uint16_t *orientation_rank)
{
    uint8_t smaller[6];
    for (uint8_t i = 0; i < 6; ++i) {
        smaller[i] = 0;
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j)
            smaller[i] += (uint8_t) (state->p[j] < state->p[i]);
    }

    uint32_t p = smaller[0];
    p = (p << 2) + (p << 1) + smaller[1];
    p = (p << 2) + p + smaller[2];
    p = (p << 2) + smaller[3];
    p = (p << 1) + p + smaller[4];
    p = (p << 1) + smaller[5];

    uint32_t o = 0;
    for (uint8_t i = 0; i < 6; ++i)
        o = (o << 1) + o + state->o[i];

    *permutation_rank = (uint16_t) p;
    *orientation_rank = (uint16_t) o;
}

#ifndef MINIRUBIK_TARGET
/*@ requires \valid(state); requires rank < STATES; assigns *state; */
static void unrank_state(uint32_t rank, state_t *state)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS, f = 720;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t q = (uint8_t) (p / f);
        p %= f;
        state->p[i] = available[q];
        for (uint8_t j = q; j + 1U < CUBIES - i; ++j)
            available[j] = available[j + 1U];
        if (i < 5)
            f /= 6U - i;
    }
    for (uint8_t i = 6; i-- > 0;) {
        state->o[i] = (uint8_t) (o % 3U);
        sum = (uint8_t) (sum + state->o[i]);
        o /= 3U;
    }
    state->o[6] = (uint8_t) ((3U - sum % 3U) % 3U);
}
#endif

/*@ requires \valid_read(state);
    requires \initialized(&state->p[0..6]) && \initialized(&state->o[0..6]);
    assigns \nothing;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures complete: valid_state(state) ==> \result != 0;
 */
static int valid(const state_t *state)
{
    uint8_t sum = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant sum <= 2 * i;
        loop invariant sum == (i > 0 ? state->o[0] : 0) +
          (i > 1 ? state->o[1] : 0) + (i > 2 ? state->o[2] : 0) +
          (i > 3 ? state->o[3] : 0) + (i > 4 ? state->o[4] : 0) +
          (i > 5 ? state->o[5] : 0) + (i > 6 ? state->o[6] : 0);
        loop invariant \forall integer j; 0 <= j < i ==>
          state->p[j] < CUBIES && state->o[j] < 3;
        loop invariant \forall integer j, k; 0 <= j < k < i ==>
          state->p[j] != state->p[k];
        loop assigns i, sum;
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (state->p[i] >= CUBIES || state->o[i] >= 3)
            return 0;
        /*@ loop invariant 0 <= j <= i;
            loop invariant \forall integer k; 0 <= k < j ==>
              state->p[k] != state->p[i];
            loop assigns j;
            loop variant i - j;
        */
        for (uint8_t j = 0; j < i; ++j)
            if (state->p[j] == state->p[i])
                return 0;
        sum = (uint8_t) (sum + state->o[i]);
    }
    return sum == 0 || sum == 3 || sum == 6 || sum == 9 || sum == 12;
}

#ifndef MINIRUBIK_TARGET
static void build_transitions(void)
{
    state_t state;

    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t) rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            permutation[face][rank] =
                (uint16_t) (rank_state(&next) / ORIENTATIONS);
        }
    }

    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orientation[face][rank] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }
}

static int build_projection_distances(
    uint16_t count, uint16_t transitions[3][count], uint8_t *distance)
{
    uint16_t queue[PERMUTATIONS];
    uint32_t head = 0, tail = 1;

    memset(distance, UINT8_MAX, count);
    distance[0] = 0;
    queue[0] = 0;

    while (head < tail) {
        uint16_t here = queue[head++];
        if (distance[here] >= UINT8_MAX - 1U)
            return 0;
        uint8_t next_distance = (uint8_t) (distance[here] + 1U);

        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next = here;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next = transitions[face][next];
                if (next >= count)
                    return 0;
                if (distance[next] == UINT8_MAX) {
                    if (tail >= count)
                        return 0;
                    distance[next] = next_distance;
                    queue[tail++] = next;
                }
            }
        }
    }

    if (tail != count || distance[0] != 0)
        return 0;
    for (uint16_t i = 0; i < count; ++i)
        if (distance[i] == UINT8_MAX)
            return 0;
    return 1;
}

static int build_heuristics(void)
{
    return build_projection_distances(
               PERMUTATIONS, permutation, perm_distance) &&
           build_projection_distances(
               ORIENTATIONS, orientation, ori_distance);
}
#endif

static uint8_t heuristic(uint16_t p, uint16_t o)
{
    uint8_t hp = perm_distance[p], ho = ori_distance[o];
    return hp > ho ? hp : ho;
}

typedef struct {
    uint16_t p, o, next_p, next_o;
    uint8_t face, turn, previous_face;
} search_frame_t;

static int ida_search(uint16_t root_p, uint16_t root_o,
                      uint8_t solution[11])
{
    search_frame_t frames[12];

    for (unsigned bound = heuristic(root_p, root_o); bound <= 11; ++bound) {
        unsigned depth = 0;
        frames[0] = (search_frame_t) {
            root_p, root_o, root_p, root_o, 0, 0, 3
        };

        for (;;) {
            search_frame_t *frame = &frames[depth];
            if (frame->p == 0 && frame->o == 0)
                return (int) depth;

            if (depth == bound || frame->face >= 3) {
                if (depth == 0)
                    break;
                --depth;
                continue;
            }

            if (frame->face == frame->previous_face) {
                ++frame->face;
                frame->turn = 0;
                frame->next_p = frame->p;
                frame->next_o = frame->o;
                continue;
            }

            uint8_t face = frame->face, turn = frame->turn;
            uint16_t child_p = permutation[face][frame->next_p];
            uint16_t child_o = orientation[face][frame->next_o];

            frame->next_p = child_p;
            frame->next_o = child_o;
            ++frame->turn;
            if (frame->turn == 3) {
                ++frame->face;
                frame->turn = 0;
                frame->next_p = frame->p;
                frame->next_o = frame->o;
            }

            if (depth + 1U + heuristic(child_p, child_o) > bound)
                continue;

            solution[depth] = (uint8_t) ((face << 1) + face + turn);
            ++depth;
            frames[depth] = (search_frame_t) {
                child_p, child_o, child_p, child_o, 0, 0, face
            };
        }
    }
    return -1;
}

static int path_solves(state_t state, const uint8_t *path, int length)
{
    if (length < 0 || length > 11)
        return 0;
    for (int i = 0; i < length; ++i) {
        if (path[i] >= MOVES)
            return 0;
        state = apply_move(state, path[i]);
    }
    for (uint8_t i = 0; i < CUBIES; ++i)
        if (state.p[i] != i || state.o[i] != 0)
            return 0;
    return 1;
}

#ifndef MINIRUBIK_TARGET
static uint8_t *build_table(uint8_t *diameter)
{
    uint8_t *toward_solved = malloc(STATES);
    uint32_t *queue = malloc((size_t) STATES * sizeof *queue);
    uint32_t head = 0, tail = 1, level_end = 1;
    if (!toward_solved || !queue) {
        free(toward_solved);
        free(queue);
        return NULL;
    }
    build_transitions();
    memset(toward_solved, UINT8_MAX, STATES);
    queue[0] = 0;
    toward_solved[0] = 0;
    *diameter = 0;
    while (head < tail) {
        if (head == level_end) {
            level_end = tail;
            ++*diameter;
        }
        uint32_t here = queue[head++];
        uint16_t p = (uint16_t) (here / ORIENTATIONS);
        uint16_t o = (uint16_t) (here % ORIENTATIONS);
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next_p = p, next_o = o;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next_p = permutation[face][next_p];
                next_o = orientation[face][next_o];
                uint32_t there = (uint32_t) next_p * ORIENTATIONS + next_o;
                if (toward_solved[there] == UINT8_MAX) {
                    uint8_t move = (uint8_t) (face * 3U + turn);
                    toward_solved[there] = inverse_move[move];
                    queue[tail++] = there;
                }
            }
        }
    }
    free(queue);
    if (tail != STATES) {
        free(toward_solved);
        return NULL;
    }
    return toward_solved;
}
#endif

/*@ requires valid_read_string(input);
    requires \valid(state);
    assigns state->p[0..6], state->o[0..6];
    ensures \result != 0 ==> input[14] == '\0';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] == input[i] - '1';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->o[i] == input[i + CUBIES] - '1';
 */
static int parse_state(const char *input, state_t *state)
{
    /*@ loop invariant 0 <= i <= 14;
        loop invariant i <= strlen(input);
        loop invariant i <= 7 ==> \initialized(&state->p[0..i-1]);
        loop invariant i >= 7 ==> \initialized(&state->p[0..6]);
        loop invariant i >= 7 ==> \initialized(&state->o[0..i-8]);
        loop invariant \forall integer j; 0 <= j < i && j < CUBIES ==>
          state->p[j] == input[j] - '1';
        loop invariant \forall integer j; 0 <= j < i - CUBIES ==>
          state->o[j] == input[j + CUBIES] - '1';
        loop assigns i, state->p[0..6], state->o[0..6];
        loop variant 14 - i;
     */
    for (int i = 0; i < 14; ++i) {
        int limit = i < 7 ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit)
            return 0;
        if (i < 7)
            state->p[i] = (uint8_t) (input[i] - '1');
        else
            state->o[i - 7] = (uint8_t) (input[i] - '1');
    }
    return input[14] == '\0' && valid(state);
}

#ifndef MINIRUBIK_TARGET
/* stdout is fully buffered off a terminal, so a write error surfaces at the
 * flush, not at the printf that queued the bytes. Every exit path that has
 * produced output goes through here.
 */
static int output_failed(void)
{
    return fflush(stdout) != 0 || ferror(stdout);
}

static int self_test(void)
{
    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    state_t state;
    for (uint8_t move = 0; move < MOVES; ++move) {
        state = solved;
        state = apply_move(state, move);
        state = apply_move(state, inverse_move[move]);
        if (memcmp(&solved, &state, sizeof solved))
            return 0;
    }
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank)
            return 0;

        uint16_t p, o;
        rank_coordinates(&state, &p, &o);
        if (p >= PERMUTATIONS || o >= ORIENTATIONS ||
            (uint32_t) p * ORIENTATIONS + o != rank)
            return 0;
    }
    puts("ranking coordinates matched all 3674160 states");
    return 1;
}

static int test_heuristics(void)
{
    uint8_t diameter;
    uint8_t *exact_moves = build_table(&diameter);
    if (!exact_moves)
        return 0;
    if (diameter != 11 || !build_heuristics()) {
        free(exact_moves);
        return 0;
    }

    uint8_t max_p = 0, max_o = 0;
    for (uint16_t p = 0; p < PERMUTATIONS; ++p)
        if (perm_distance[p] > max_p)
            max_p = perm_distance[p];
    for (uint16_t o = 0; o < ORIENTATIONS; ++o)
        if (ori_distance[o] > max_o)
            max_o = ori_distance[o];

    printf("H2 permutation: states=%u solved=%u max=%u\n",
           (unsigned) PERMUTATIONS, (unsigned) perm_distance[0],
           (unsigned) max_p);
    printf("H2 orientation: states=%u solved=%u max=%u\n",
           (unsigned) ORIENTATIONS, (unsigned) ori_distance[0],
           (unsigned) max_o);

    for (uint32_t rank = 0; rank < STATES; ++rank) {
        uint16_t p = (uint16_t) (rank / ORIENTATIONS);
        uint16_t o = (uint16_t) (rank % ORIENTATIONS);
        uint8_t h = heuristic(p, o), distance = 0;

        while (p != 0 || o != 0) {
            if (distance >= diameter) {
                free(exact_moves);
                return 0;
            }
            uint8_t move =
                exact_moves[(uint32_t) p * ORIENTATIONS + o];
            if (move >= MOVES) {
                free(exact_moves);
                return 0;
            }
            uint8_t face = (uint8_t) (move / 3U);
            uint8_t turns = (uint8_t) (move % 3U + 1U);
            for (uint8_t turn = 0; turn < turns; ++turn) {
                p = permutation[face][p];
                o = orientation[face][o];
            }
            ++distance;
        }

        if (h > distance) {
            fprintf(stderr, "H1 failed: rank=%u h=%u d=%u\n",
                    (unsigned) rank, (unsigned) h,
                    (unsigned) distance);
            free(exact_moves);
            return 0;
        }
    }

    free(exact_moves);
    puts("H1 passed: 3674160 states checked");
    return 1;
}

static int exact_distance(const uint8_t *table, uint16_t p, uint16_t o)
{
    int distance = 0;
    while (p != 0 || o != 0) {
        if (distance >= 11)
            return -1;
        uint8_t move = table[(uint32_t) p * ORIENTATIONS + o];
        if (move >= MOVES)
            return -1;
        uint8_t face = (uint8_t) (move / 3U);
        uint8_t turns = (uint8_t) (move % 3U + 1U);
        for (uint8_t turn = 0; turn < turns; ++turn) {
            p = permutation[face][p];
            o = orientation[face][o];
        }
        ++distance;
    }
    return distance;
}

static int check_search_case(state_t state, const uint8_t *table)
{
    uint32_t rank = rank_state(&state);
    uint16_t p = (uint16_t) (rank / ORIENTATIONS);
    uint16_t o = (uint16_t) (rank % ORIENTATIONS);
    uint8_t path[11];
    int expected = exact_distance(table, p, o);
    int length = ida_search(p, o, path);
    if (expected < 0 || length != expected ||
        !path_solves(state, path, length)) {
        fprintf(stderr, "search mismatch: rank=%u expected=%d got=%d\n",
                (unsigned) rank, expected, length);
        return 0;
    }
    return 1;
}

static int test_search(void)
{
    uint8_t diameter;
    uint8_t *table = build_table(&diameter);
    if (!table)
        return 0;
    if (diameter != 11 || !build_heuristics()) {
        free(table);
        return 0;
    }

    FILE *vectors = fopen("tests/solutions.txt", "r");
    if (!vectors) {
        free(table);
        return 0;
    }

    char line[256];
    unsigned count = 0;
    int ok = 1;
    while (fgets(line, sizeof line, vectors)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        char *separator = strchr(line, '|');
        state_t state;
        if (!separator) {
            ok = 0;
            break;
        }
        *separator = '\0';
        if (!parse_state(line, &state) || !check_search_case(state, table)) {
            ok = 0;
            break;
        }
        ++count;
    }
    if (ferror(vectors))
        ok = 0;
    fclose(vectors);
    if (!ok || count == 0) {
        free(table);
        return 0;
    }

    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    if (!check_search_case(solved, table)) {
        free(table);
        return 0;
    }
    for (uint8_t move = 0; move < MOVES; ++move) {
        if (!check_search_case(apply_move(solved, move), table)) {
            free(table);
            return 0;
        }
    }
    state_t short_scramble = apply_move(apply_move(solved, 0), 3);
    if (!check_search_case(short_scramble, table)) {
        free(table);
        return 0;
    }

    char short_input[15];
    for (uint8_t i = 0; i < CUBIES; ++i) {
        short_input[i] = (char) ('1' + short_scramble.p[i]);
        short_input[i + CUBIES] = (char) ('1' + short_scramble.o[i]);
    }
    short_input[14] = '\0';

    free(table);
    printf("%u vectors matched exact BFS lengths and replayed successfully\n",
           count);
    puts("solved, 9 one-move states, and short scramble R B passed");
    printf("short scramble input: %s\n", short_input);
    printf("search frame buffers: %u bytes\n",
           (unsigned) (12U * sizeof(search_frame_t)));
    return 1;
}

static int test_full_search(void)
{
    uint8_t diameter;
    uint8_t *table = build_table(&diameter);
    if (!table) {
        fputs("could not build complete BFS table\n", stderr);
        return 0;
    }
    if (diameter != 11 || !build_heuristics()) {
        free(table);
        return 0;
    }

    uint32_t distance11 = 0;
    puts("H3 started: checking all states against exact BFS distances");
    fflush(stdout);

    for (uint32_t rank = 0; rank < STATES; ++rank) {
        state_t state;
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank) {
            fprintf(stderr, "H3 state decoding failed: rank=%u\n",
                    (unsigned) rank);
            free(table);
            return 0;
        }
        uint16_t p = (uint16_t) (rank / ORIENTATIONS);
        uint16_t o = (uint16_t) (rank % ORIENTATIONS);
        uint8_t path[11];
        int expected = exact_distance(table, p, o);
        if (expected < 0) {
            fprintf(stderr, "H3 BFS reference failed: rank=%u\n",
                    (unsigned) rank);
            free(table);
            return 0;
        }
        int length = ida_search(p, o, path);

        if (length != expected || !path_solves(state, path, length)) {
            fprintf(stderr, "H3 failed: rank=%u expected=%d got=%d\n",
                    (unsigned) rank, expected, length);
            free(table);
            return 0;
        }
        if (expected == 11)
            ++distance11;

        uint32_t checked = rank + 1U;
        if (checked % 65536U == 0 || checked == STATES) {
            printf("H3 progress: %u/%u states\n",
                   (unsigned) checked, (unsigned) STATES);
            fflush(stdout);
        }
    }

    free(table);
    if (distance11 != 2644U) {
        fprintf(stderr, "unexpected distance-11 count: %u\n",
                (unsigned) distance11);
        return 0;
    }
    printf("distance-11 states checked: %u\n", (unsigned) distance11);
    printf("H3 passed: %u optimal lengths and replayed paths\n",
           (unsigned) STATES);
    return 1;
}

int main(int argc, char **argv)
{
    state_t state;
    uint8_t diameter;
    if (argc == 2 && !strcmp(argv[1], "--full-search-test")) {
        if (!test_full_search())
            return 1;
        return output_failed();
    }    
    if (argc == 2 && !strcmp(argv[1], "--search-test")) {
        if (!test_search()) {
            fputs("search-test failed\n", stderr);
            return 1;
        }
        return output_failed();
    } 
    if (argc == 2 && !strcmp(argv[1], "--heuristic-test")) {
        if (!test_heuristics()) {
            fputs("heuristic-test failed\n", stderr);
            return 1;
        }
        return output_failed();
    }
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        if (!self_test()) {
            fputs("self-test failed\n", stderr);
            return 1;
        }
        uint8_t *table = build_table(&diameter);
        if (!table) {
            fputs("could not build complete state table\n", stderr);
            return 1;
        }
        free(table);
        if (diameter != 11) {
            fputs("BFS check failed\n", stderr);
            return 1;
        }
        puts("3674160 states; diameter 11");
        return output_failed();
    }
    if (argc != 2 || !parse_state(argv[1], &state)) {
        /* C99 5.1.2.2.1 lets argv[0] be null when argc is 0. */
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n",
                argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }

    build_transitions();
    if (!build_heuristics()) {
        fputs("could not build heuristic tables\n", stderr);
        return 1;
    }

    uint16_t p, o;
    rank_coordinates(&state, &p, &o);
    uint8_t path[11];
    int length = ida_search(p, o, path);
    if (!path_solves(state, path, length)) {
        fputs("search or path validation failed\n", stderr);
        return 1;
    }

    for (int i = 0; i < length; ++i)
        printf("%s%s", i ? " " : "", move_names[path[i]]);
    putchar('\n');
    return output_failed();
}
#endif
