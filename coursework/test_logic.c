#include <assert.h>
#include <stdio.h>
#include "guess_logic.h"

int main(void)
{
    struct guess_state s;
    int secret, low, high, value;
    for (secret = 1; secret <= 100; secret++) {
        guess_start(&s, secret);
        assert(guess_submit(&s, 0) == -1 && s.attempts == 0);
        assert(guess_submit(&s, 101) == -1 && s.attempts == 0);
        low = 1; high = 100;
        while (s.status != GUESS_WON && low <= high) {
            value = low + (high - low) / 2;
            assert(guess_submit(&s, value) == 0);
            if (s.status == GUESS_HIGHER) low = value + 1;
            if (s.status == GUESS_LOWER) high = value - 1;
            assert(s.attempts <= 7);
        }
        assert(s.status == GUESS_WON && s.last == secret);
        value = s.attempts;
        assert(guess_submit(&s, secret) == -2 && s.attempts == (unsigned int)value);
        guess_start(&s, secret);
        assert(s.attempts == 0 && s.status == GUESS_READY && s.last == 0);
    }
    puts("PASS: all 100 secrets solved in <=7 guesses; bounds, completed rounds and reset verified.");
    return 0;
}
