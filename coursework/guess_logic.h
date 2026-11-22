/* SPDX-License-Identifier: GPL-2.0 */
#ifndef COURSE_GUESS_LOGIC_H
#define COURSE_GUESS_LOGIC_H

#define GUESS_MIN 1
#define GUESS_MAX 100
enum guess_status { GUESS_READY, GUESS_HIGHER, GUESS_LOWER, GUESS_WON };
struct guess_state {
    int secret;
    int last;
    unsigned int attempts;
    enum guess_status status;
};

static inline void guess_start(struct guess_state *s, int secret)
{
    s->secret = secret;
    s->last = 0;
    s->attempts = 0;
    s->status = GUESS_READY;
}

/* Invalid or completed-round guesses must not change state. */
static inline int guess_submit(struct guess_state *s, int value)
{
    if (value < GUESS_MIN || value > GUESS_MAX)
        return -1;
    if (s->status == GUESS_WON)
        return -2;
    s->last = value;
    s->attempts++;
    s->status = value < s->secret ? GUESS_HIGHER :
                value > s->secret ? GUESS_LOWER : GUESS_WON;
    return 0;
}

static inline const char *guess_status_name(enum guess_status status)
{
    switch (status) {
    case GUESS_HIGHER: return "higher";
    case GUESS_LOWER: return "lower";
    case GUESS_WON: return "won";
    default: return "ready";
    }
}
#endif
