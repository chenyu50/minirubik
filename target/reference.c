#define MINIRUBIK_TARGET
#include "../solver.c"

extern const char input_state[];

static void print_text(const char *text)
{
    register const char *a0 __asm__("a0") = text;
    register uint32_t a7 __asm__("a7") = 4;
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
}

int main(void)
{
    state_t state;
    if (!parse_state(input_state, &state)) {
        print_text("invalid input\n");
        return 2;
    }

    uint16_t p, o;
    rank_coordinates(&state, &p, &o);

    uint8_t path[11];
    int length = ida_search(p, o, path);
    if (!path_solves(state, path, length)) {
        print_text("search or path validation failed\n");
        return 1;
    }

    for (int i = 0; i < length; ++i) {
        if (i)
            print_text(" ");
        print_text(move_names[path[i]]);
    }
    print_text("\n");
    return 0;
}
