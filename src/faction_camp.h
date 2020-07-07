#pragma once
#ifndef CATA_SRC_FACTION_CAMP_H
#define CATA_SRC_FACTION_CAMP_H

#include <string>
#include <vector>
#include <utility>

#include "basecamp.h"

namespace catacurses
{
class window;
} // namespace catacurses
class npc;
struct point;
struct tripoint;
struct mission_entry;

enum class farm_ops : int {
    plow = 1,
    plant = 2,
    harvest = 4
};
inline bool operator&( const farm_ops &rhs, const farm_ops &lhs )
{
    return static_cast<int>( rhs ) & static_cast<int>( lhs );
}

std::string get_mission_action_string( const std::string &input_mission );

namespace talk_function
{
void basecamp_mission( npc & );

///Changes an NPC follower to a camp manager
void become_overseer( npc & );
///Changes an NPC follower to a camp manager, displays camp warnings, and sets the current OM tile to a camp survey
void start_camp( npc & );
///Changes an NPC follower to a camp manager of an existing camp.
void recover_camp( npc & );
///Changes an NPC camp manager to a follower
void remove_overseer( npc & );

void draw_camp_tabs( const catacurses::window &win, base_camps::tab_mode cur_tab,
                     const std::vector<std::vector<mission_entry>> &entries );
std::string name_mission_tabs( const tripoint &omt_pos, const std::string &role_id,
                               const std::string &cur_title, base_camps::tab_mode cur_tab );

/// Returns the OM tiles surrounding the camp, @ref purge removes all tiles that aren't expansions
std::vector<std::pair<std::string, tripoint>> om_building_region( const tripoint &omt_pos,
        int range, bool purge = false );
/// Returns the x and y coordinates of ( omt_tar - omt_pos ), clamped to [-1, 1]
point om_simple_dir( const tripoint &omt_pos, const tripoint &omt_tar );
} // namespace talk_function
#endif // CATA_SRC_FACTION_CAMP_H
