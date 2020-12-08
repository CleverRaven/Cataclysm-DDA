#ifndef CATA_SRC_CACHED_OPTIONS_H
#define CATA_SRC_CACHED_OPTIONS_H

// A collection of options which are accessed frequently enough that we don't
// want to pay the overhead of a string lookup each time one is tested.
// They should be updated when the corresponding option is changed (in
// options.cpp).

extern bool fov_3d;
extern int fov_3d_z_range;
extern bool keycode_mode;
extern bool log_from_top;
extern int message_ttl;
extern int message_cooldown;
extern bool tile_iso;
extern bool use_tiles;

// test_mode is not a regular game option; it's true when we are running unit
// tests.
extern bool test_mode;
enum class test_mode_spilling_action_t {
    spill_all,
    cancel_spill,
};
extern test_mode_spilling_action_t test_mode_spilling_action;

extern bool direct3d_mode;

#endif // CATA_SRC_CACHED_OPTIONS_H
