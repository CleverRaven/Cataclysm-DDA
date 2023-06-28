#include "cached_options.h"

bool fov_3d;
int fov_3d_z_range;
bool keycode_mode;
bool log_from_top;
int message_ttl;
int message_cooldown;
bool test_mode;
int prevent_occlusion;
bool prevent_occlusion_retract;
bool prevent_occlusion_transp;
float prevent_occlusion_min_dist;
float prevent_occlusion_max_dist;
bool use_tiles;
bool use_far_tiles;
bool use_pinyin_search;
bool use_tiles_overmap;
test_mode_spilling_action_t test_mode_spilling_action = test_mode_spilling_action_t::spill_all;
bool direct3d_mode;
bool pixel_minimap_option;
int pixel_minimap_r;
int pixel_minimap_g;
int pixel_minimap_b;
int pixel_minimap_a;

namespace cata::options
{
std::vector<std::string> damage_indicators;
} // namespace cata::options

#ifndef CATA_IN_TOOL
error_log_format_t error_log_format = error_log_format_t::human_readable;
check_plural_t check_plural = check_plural_t::certain;
#endif
