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
extern int tile_retracted;
extern float tile_retract_dist_min;
extern float tile_retract_dist_max;
extern bool use_tiles;
extern bool use_far_tiles;
extern bool use_tiles_overmap;
extern bool pixel_minimap_option;
extern int pixel_minimap_r;
extern int pixel_minimap_g;
extern int pixel_minimap_b;
extern int pixel_minimap_a;

// test_mode is not a regular game option; it's true when we are running unit
// tests.
extern bool test_mode;
enum class test_mode_spilling_action_t {
    spill_all,
    cancel_spill,
};
extern test_mode_spilling_action_t test_mode_spilling_action;

extern bool direct3d_mode;

enum class error_log_format_t {
    human_readable,
    // Output error messages in github action command format (currently json only)
    // See https://docs.github.com/en/free-pro-team@latest/actions/reference/workflow-commands-for-github-actions#setting-an-error-message
    github_action,
};
#ifndef CATA_IN_TOOL
extern error_log_format_t error_log_format;
#else
constexpr error_log_format_t error_log_format = error_log_format_t::human_readable;
#endif

enum class check_plural_t {
    none,
    certain, // report strings that certainly have a non-regular plural form, such as those ending in "s"
    possible, // report strings that may or may not have a non-regular plural form, such as those containing the word "of"
};
#ifndef CATA_IN_TOOL
extern check_plural_t check_plural;
#else
constexpr check_plural_t check_plural = check_plural_t::none;
#endif

#endif // CATA_SRC_CACHED_OPTIONS_H
