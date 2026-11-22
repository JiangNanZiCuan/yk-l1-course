/* SPDX-License-Identifier: GPL-2.0 */
/* Userspace player. Both automatic and manual modes save actual kernel feedback. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#define GAME_PATH "/proc/guess_game"
#define REPORT_PATH "/tmp/guess_result.txt"
struct feedback { unsigned int round, attempts; int last; char status[16]; };

static int send_command(const char *command)
{
    FILE *f = fopen(GAME_PATH, "w");
    int failed;
    if (!f) { perror(GAME_PATH); return -1; }
    failed = fprintf(f, "%s\n", command) < 0;
    if (fclose(f)) failed = 1;
    if (failed) { perror("kernel rejected command"); return -1; }
    return 0;
}

static int read_feedback(struct feedback *s)
{
    FILE *f = fopen(GAME_PATH, "r");
    char line[128];
    unsigned int seen = 0;
    memset(s, 0, sizeof(*s));
    if (!f) { perror(GAME_PATH); return -1; }
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "round=%u", &s->round) == 1) seen |= 1;
        else if (sscanf(line, "attempts=%u", &s->attempts) == 1) seen |= 2;
        else if (sscanf(line, "last=%d", &s->last) == 1) seen |= 4;
        else if (sscanf(line, "status=%15s", s->status) == 1) seen |= 8;
    }
    if (ferror(f)) seen = 0;
    fclose(f);
    if (seen != 15) { fprintf(stderr, "Incomplete kernel response\n"); return -1; }
    return 0;
}

static void record(FILE *report, const char *format, ...)
{
    va_list args;
    va_start(args, format); vprintf(format, args); va_end(args);
    va_start(args, format); vfprintf(report, format, args); va_end(args);
}

static void show(FILE *report, const struct feedback *s)
{
    record(report, "round=%u attempt=%u guess=%d result=%s\n",
           s->round, s->attempts, s->last, s->status);
}

static int automatic(FILE *report)
{
    struct feedback s;
    int low = 1, high = 100, value, turn;
    char command[16];
    unsigned int round;
    if (send_command("new") || read_feedback(&s)) return 1;
    round = s.round;
    record(report, "Mode: binary search; range: 1..100\n");
    for (turn = 1; turn <= 7 && low <= high; turn++) {
        value = low + (high - low) / 2;
        snprintf(command, sizeof(command), "%d", value);
        if (send_command(command) || read_feedback(&s)) return 1;
        if (s.round != round || s.attempts != (unsigned int)turn || s.last != value) {
            record(report, "Round changed by another player; stopped.\n");
            return 1;
        }
        show(report, &s);
        if (!strcmp(s.status, "won")) {
            record(report, "Solved in %u guesses.\n", s.attempts);
            return 0;
        }
        if (!strcmp(s.status, "higher")) low = value + 1;
        else if (!strcmp(s.status, "lower")) high = value - 1;
        else break;
    }
    record(report, "Unexpected feedback; game did not finish.\n");
    return 1;
}

int main(int argc, char **argv)
{
    FILE *report;
    struct feedback state;
    time_t now;
    char *end;
    long value;
    int result = 0;
    const char *mode = argc == 1 ? "--auto" : argv[1];
    if ((!strcmp(mode, "--auto") || !strcmp(mode, "--new") ||
         !strcmp(mode, "--status")) ? argc > 2 :
        (strcmp(mode, "--guess") || argc != 3)) {
        fprintf(stderr, "Usage: guess_client [--auto | --new | --guess 1..100 | --status]\n");
        return 2;
    }
    if (!strcmp(mode, "--guess")) {
        errno = 0;
        value = strtol(argv[2], &end, 10);
        if (errno || end == argv[2] || *end || value < 1 || value > 100) {
            fprintf(stderr, "Guess must be an integer from 1 to 100.\n"); return 2;
        }
    }
    report = fopen(REPORT_PATH, !strcmp(mode, "--auto") || !strcmp(mode, "--new") ? "w" : "a");
    if (!report) { perror(REPORT_PATH); return 1; }
    now = time(NULL);
    record(report, "=== Kernel Guess Game ===\nTime: %s", ctime(&now));
    if (!strcmp(mode, "--auto")) result = automatic(report);
    else {
        if (!strcmp(mode, "--new")) result = send_command("new") != 0;
        else if (!strcmp(mode, "--guess")) result = send_command(argv[2]) != 0;
        if (!result) {
            result = read_feedback(&state) != 0;
            if (!result) show(report, &state);
        }
    }
    record(report, "Result file: %s\nExit status: %d\n", REPORT_PATH, result);
    if (ferror(report)) result = 1;
    if (fclose(report)) result = 1;
    return result;
}
