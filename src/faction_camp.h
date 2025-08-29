#pragma once
#ifndef CATA_SRC_FACTION_CAMP_H
#define CATA_SRC_FACTION_CAMP_H

#include <string>
#include <utility>
#include <vector>

#include "coords_fwd.h"

template <typename E> struct enum_traits;

namespace catacurses
{
class window;
} // namespace catacurses
class npc;
struct mission_entry;

namespace base_camps
{
enum tab_mode : int;
} // namespace base_camps

enum class farm_ops : int {
    plow = 1,
    plant = 2,
    harvest = 4
};

template<>
struct enum_traits<farm_ops> {
    static constexpr bool is_flag_enum = true;
};

namespace talk_function
{
void basecamp_mission( npc & );

/// Start a faction camp on the current OM tile
void start_camp( npc & );

void draw_camp_tabs( const catacurses::window &win, base_camps::tab_mode cur_tab,
                     const std::vector<std::vector<mission_entry>> &entries );
std::string name_mission_tabs( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                               const std::string &cur_title, base_camps::tab_mode cur_tab );

/// Returns the OM tiles surrounding the camp, @ref purge removes all tiles that aren't expansions
std::vector<std::pair<std::string, tripoint_abs_omt>> om_building_region(
            const tripoint_abs_omt &omt_pos, int range, bool purge = false );
/// Returns the x and y coordinates of ( omt_tar - omt_pos ), clamped to [-1, 1]
point_rel_omt om_simple_dir( const tripoint_abs_omt &omt_pos, const tripoint_abs_omt &omt_tar );
} // namespace talk_function
#endif // CATA_SRC_FACTION_CAMP_H
