#ifndef CATA_SRC_CACHED_OPTIONS_H
#define CATA_SRC_CACHED_OPTIONS_H

// A collection of options which are accessed frequently enough that we don't
// want to pay the overhead of a string lookup each time one is tested.
// They should be updated when the corresponding option is changed (in
// options.cpp).

extern bool keycode_mode;
// sidebar messages flow direction
extern bool log_from_top;
extern int message_ttl;
extern int message_cooldown;

// test_mode is not a regular game option; it's true when we are running unit
// tests.
extern bool test_mode;

#endif // CATA_SRC_CACHED_OPTIONS_H
