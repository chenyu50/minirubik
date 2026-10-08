#define _POSIX_C_SOURCE 200809L
#define main minirubik_benchmark_main
#include "../benchmark.c"
#undef main
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define CASES 2644U
#define LIMIT 50000000ULL
#define CC "riscv64-unknown-elf-gcc"
#define OUT "measurements/distance11/"
#define BUILD "build/distance11/"
static char inputs[CASES][15];

static int gate_run(const char *const command[], const char *log)
{
    pid_t pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0) {
        int fd = open(log, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0 ||
            dup2(fd, STDERR_FILENO) < 0)
            _exit(126);
        close(fd);
        execvp(command[0], (char *const *) command);
        _exit(127);
    }
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR)
            return -1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static int gate_fail(const char *input, const char *phase)
{
    fprintf(stderr, "FAIL: input=%s phase=%s\n", input, phase);
    FILE *file = fopen(OUT "summary.txt", "w");
    if (file) {
        fprintf(file, "INCOMPLETE: input=%s phase=%s\n", input, phase);
        fclose(file);
    }
    return 1;
}

static int gate_mkdir(const char *path)
{
    return mkdir(path, 0777) == 0 || errno == EEXIST;
}

static int gate_load_inputs(void)
{
    FILE *file = fopen("tests/distance11.txt", "r");
    if (!file)
        return 0;
    char line[64];
    unsigned count = 0;
    int fixed = 0, good = 1;
    while (fgets(line, sizeof line, file)) {
        size_t length = strlen(line);
        while (length && (line[length - 1] == '\n' ||
                          line[length - 1] == '\r'))
            line[--length] = '\0';
        state_t state;
        if (length != 14 || count >= CASES || !parse_state(line, &state)) {
            good = 0;
            break;
        }
        for (unsigned i = 0; i < count; ++i)
            if (!strcmp(inputs[i], line))
                good = 0;
        if (!good)
            break;
        memcpy(inputs[count++], line, 15);
        if (!strcmp(line, "21345671111111"))
            fixed = 1;
    }
    if (ferror(file))
        good = 0;
    if (fclose(file) != 0)
        good = 0;
    return good && count == CASES && fixed;
}

static int gate_clean_sources(void)
{
    const char *command[] = {
        "git", "diff", "--exit-code", "HEAD", "--",
        "target/", "generated/tables.inc", "benchmark.c",
        "tools/export_hard_states.c", "tools/check_hard_states.c",
        "tests/distance11.txt", NULL
    };
    return gate_run(command, OUT "source-changes.log") == 0;
}

static int gate_same_files(const char *a, const char *b)
{
    FILE *first = fopen(a, "rb"), *second = fopen(b, "rb");
    if (!first || !second) {
        if (first)
            fclose(first);
        if (second)
            fclose(second);
        return 0;
    }
    int x, y, equal = 1;
    do {
        x = fgetc(first);
        y = fgetc(second);
        if (x != y)
            equal = 0;
    } while (equal && x != EOF);
    if (ferror(first) || ferror(second))
        equal = 0;
    if (fclose(first) != 0)
        equal = 0;
    if (fclose(second) != 0)
        equal = 0;
    return equal;
}

static int gate_compile(const char *source_file, const char *object,
                        const char *define_input)
{
    const char *command[] = {
        CC, "-march=rv32i", "-mabi=ilp32", "-mno-relax", "-nostdlib",
        "-c", source_file, "-o", object, define_input, NULL
    };
    return gate_run(command, OUT "build.log") == 0;
}

static int gate_read_log(const char *raw, const char *readable,
                         char *text, size_t capacity)
{
    FILE *file = fopen(raw, "rb");
    if (!file)
        return 0;
    size_t length = 0;
    int byte, good = 1;
    while ((byte = fgetc(file)) != EOF) {
        if (byte == 0)
            continue;
        if (length + 1 >= capacity) {
            good = 0;
            break;
        }
        text[length++] = (char) byte;
    }
    if (ferror(file))
        good = 0;
    if (fclose(file) != 0)
        good = 0;
    text[length] = '\0';
    if (!good)
        return 0;
    file = fopen(readable, "wb");
    if (!file)
        return 0;
    good = fwrite(text, 1, length, file) == length;
    if (fclose(file) != 0)
        good = 0;
    return good;
}

static int gate_check_output(char *text, const char *input,
                             unsigned long long *instructions,
                             char solution[64])
{
    char *exit_line = strstr(text, "Program exited with code:");
    char *retired = strstr(text, "===== instructions retired");
    int guest;
    if (!exit_line || !retired ||
        sscanf(exit_line, "Program exited with code: %d", &guest) != 1 ||
        guest != 0 ||
        sscanf(retired + strlen("===== instructions retired"),
               "%llu", instructions) != 1 ||
        *instructions == 0 ||
        !strstr(text, "processor: RV32_ISS\n") ||
        !strstr(text, "ISA extensions: \n"))
        return 0;

    if (*instructions > LIMIT) {
        fprintf(stderr, "%s: %llu exceeds %llu instructions\n",
                input, *instructions, LIMIT);
        return 0;
    }
    *exit_line = '\0';
    state_t state;
    if (!parse_state(input, &state))
        return 0;
    unsigned count = 0;
    solution[0] = '\0';
    for (char *token = strtok(text, " \t\r\n"); token;
         token = strtok(NULL, " \t\r\n")) {
        unsigned move;
        for (move = 0; move < MOVES; ++move)
            if (!strcmp(token, move_names[move]))
                break;
        if (move == MOVES || count >= 11)
            return 0;
        if (count)
            strcat(solution, " ");
        strcat(solution, token);
        state = apply_move(state, (uint8_t) move);
        ++count;
    }
    if (count != 11)
        return 0;
    for (unsigned i = 0; i < CUBIES; ++i)
        if (state.p[i] != i || state.o[i] != 0)
            return 0;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("Usage: check-hard RIPES_BIN\n", stderr);
        return 2;
    }
    if (!gate_mkdir("build") || !gate_mkdir(BUILD) ||
        !gate_mkdir("measurements") || !gate_mkdir(OUT))
        return gate_fail("-", "create directories");
    FILE *initial = fopen(OUT "summary.txt", "w");
    if (!initial)
        return gate_fail("-", "open initial summary");
    fputs("INCOMPLETE: batch started\n", initial);
    int initial_good = !ferror(initial);
    if (fclose(initial) != 0 || !initial_good)
        return gate_fail("-", "write initial summary");
    if (!gate_load_inputs())
        return gate_fail("-", "expected 2644 unique valid inputs");

    const char *git[] = {"git", "rev-parse", "HEAD", NULL};
    const char *version[] = {CC, "--version", NULL};
    if (gate_run(git, OUT "source-commit.txt") != 0 ||
        gate_run(version, OUT "compiler.txt") != 0 ||
        !gate_clean_sources())
        return gate_fail("-", "record source and compiler");

    FILE *config = fopen(OUT "configuration.txt", "w");
    if (!config)
        return gate_fail("-", "open configuration");
    fprintf(config, "ripes=%s\nprocessor=RV32_ISS\nISA=RV32I\n"
            "renderer=off\nlimit=%llu\n"
            "flags=-march=rv32i -mabi=ilp32 -mno-relax -nostdlib\n",
            argv[1], LIMIT);
    int good = !ferror(config);
    if (fclose(config) != 0 || !good)
        return gate_fail("-", "write configuration");

    struct timespec begin, end;
    if (clock_gettime(CLOCK_MONOTONIC, &begin) != 0)
        return gate_fail("-", "start timer");
    const char *names[] = {"state", "moves", "search", "main"};
    for (unsigned i = 0; i < 4; ++i) {
        char source_file[64], object[64];
        snprintf(source_file, sizeof source_file, "target/%s.S", names[i]);
        snprintf(object, sizeof object, BUILD "%s.o", names[i]);
        if (!gate_compile(source_file, object, NULL))
            return gate_fail("-", "compile common modules; see build.log");
    }

    FILE *csv = fopen(OUT "results.csv", "w");
    if (!csv)
        return gate_fail("-", "open CSV");
    fputs("input,instructions,solution\n", csv);
    if (ferror(csv) || fflush(csv) != 0)
        return gate_fail("-", "write CSV header");
    unsigned long long maximum = 0, fixed_count = 0;
    char worst[15] = "";
    for (unsigned i = 0; i < CASES; ++i) {
        const char *input = inputs[i];
        char define_input[64], raw[128], readable[128];
        snprintf(define_input, sizeof define_input,
                 "-DINPUT_STATE=\"%s\"", input);
        if (!gate_compile("target/start.S", BUILD "start.o", define_input))
            return gate_fail(input, "compile input; see build.log");
        const char *link[] = {
            CC, "-march=rv32i", "-mabi=ilp32", "-mno-relax", "-nostdlib",
            BUILD "start.o", BUILD "state.o", BUILD "moves.o",
            BUILD "search.o", BUILD "main.o",
            "-Wl,--no-relax,-T,target/link.ld", "-o", BUILD "current.elf",
            NULL
        };
        if (gate_run(link, OUT "build.log") != 0)
            return gate_fail(input, "link; see build.log");
        snprintf(raw, sizeof raw, OUT "%s.log", input);
        snprintf(readable, sizeof readable, OUT "%s.txt", input);
        const char *run[] = {
            argv[1], "--mode", "cli", "--proc", "RV32_ISS",
            "--src", BUILD "current.elf", "-t", "elf",
            "--iret", "--runinfo", "--timeout", "120000", NULL
        };
        int status = gate_run(run, raw);
        FILE *log = fopen(raw, "ab");
        if (!log)
            return gate_fail(input, "append CLI status");
        fprintf(log, "\ninput=%s ripes_cli_exit=%d\n", input, status);
        good = !ferror(log);
        if (fclose(log) != 0 || !good)
            return gate_fail(input, "write CLI status");
        char text[8192], solution[64];
        if (!gate_read_log(raw, readable, text, sizeof text))
            return gate_fail(input, "read log or write readable copy");
        if (status != 0)
            return gate_fail(input, "Ripes execution incomplete; see log");
        unsigned long long instructions;
        if (!gate_check_output(text, input, &instructions, solution))
            return gate_fail(input, "exit, telemetry, length, replay, or budget");
        fprintf(csv, "%s,%llu,\"%s\"\n", input, instructions, solution);
        if (ferror(csv) || fflush(csv) != 0)
            return gate_fail(input, "write CSV row");
        if (instructions > maximum) {
            maximum = instructions;
            strcpy(worst, input);
        }
        if (!strcmp(input, "21345671111111"))
            fixed_count = instructions;
        if ((i + 1) % 25 == 0 || i + 1 == CASES) {
            printf("checked=%u/%u max=%llu\n", i + 1, CASES, maximum);
            if (fflush(stdout) != 0)
                return gate_fail(input, "write progress");
        }
    }
    if (fclose(csv) != 0)
        return gate_fail("-", "close CSV");
    if (!gate_clean_sources() ||
        gate_run(git, OUT "source-commit-after.txt") != 0 ||
        !gate_same_files(OUT "source-commit.txt", OUT "source-commit-after.txt"))
        return gate_fail("-", "sources or commit changed during batch");
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
        return gate_fail("-", "stop timer");
    double seconds = (double) (end.tv_sec - begin.tv_sec) +
                     (double) (end.tv_nsec - begin.tv_nsec) / 1000000000.0;

    FILE *summary = fopen(OUT "summary.txt", "w");
    if (!summary)
        return gate_fail("-", "open final summary");
    fprintf(summary, "PASS: all %u distance-11 inputs\n"
            "all paths: length 11; target and host replay passed\n"
            "maximum instructions: %llu\nworst input: %s\n"
            "fixed sample instructions: %llu\ninstruction limit: %llu\n"
            "batch wall seconds: %.2f\n",
            CASES, maximum, worst, fixed_count, LIMIT, seconds);
    good = !ferror(summary);
    if (fclose(summary) != 0 || !good)
        return gate_fail("-", "write final summary");
    printf("PASS: all %u distance-11 inputs\n"
           "maximum instructions: %llu\nworst input: %s\n"
           "fixed sample instructions: %llu\nbatch wall seconds: %.2f\n",
           CASES, maximum, worst, fixed_count, seconds);
    if (fflush(stdout) != 0 || ferror(stdout))
        return gate_fail("-", "write final output");
    return 0;
}
